import argparse
import json
import re
import logging
from xml.etree import ElementTree as ET
from xml.dom import minidom

def prettify(elem):
    """Return a pretty-printed XML string for the Element."""
    rough_string = ET.tostring(elem, 'utf-8')
    reparsed = minidom.parseString(rough_string)
    return "\n".join([line for line in reparsed.toprettyxml(indent="  ").splitlines() if line.strip()])

def create_parser():
    parser = argparse.ArgumentParser(description="Gem5 to McPAT XML parser")
    parser.add_argument('--config', '-c', required=True, help="Path to config.json")
    parser.add_argument('--stats', '-s', required=True, help="Path to stats.txt")
    parser.add_argument('--template', '-t', required=True, help="Path to the template XML")
    parser.add_argument('--output', '-o', default="mcpat-out.xml", help="Path for the output XML")
    return parser

def read_stats_file(stats_file):
    """Parse the stats.txt file."""
    stats = {}
    pattern = re.compile(r'(\S+)\s+(\S+)')
    with open(stats_file, 'r') as file:
        for line in file:
            match = pattern.match(line)
            if match:
                key, value = match.groups()
                stats[key] = value if value != "nan" else "0"
    return stats

def read_config_file(config_file):
    """Parse the config.json file."""
    with open(config_file, 'r') as file:
        return json.load(file)

def get_conf_value(conf_str, config):
    """Resolve a key path in the config dictionary, treating 'cpus' as a list."""
    logging.debug(f"Resolving config path: {conf_str}")
    keys = conf_str.split('.')
    value = config

    for key in keys:
        if isinstance(value, list):  # If the current level is a list
            # We assume the list contains the first relevant element
            value = value[0]  # Access the first element in the list
        elif isinstance(value, dict):  # If the current level is a dictionary
            if key in value:
                value = value[key]
            else:
                return "0"  # Default to "0" if key is not found
        else:
            return "0"  # Default to "0" if key path leads to an unsupported type

    return str(value[key])  # Return as a string

def evaluate_expression(expr, stats, config):
    """Evaluate a stat or config expression and handle arithmetic operations."""
    original_expr = expr
    logging.debug(f"Evaluating expression: {original_expr}")

    # Resolve stats values
    if expr.startswith("stats."):
        # if there is addition:
        parts = expr.split('+')
        if(len(parts) > 1):
            logging.warning(f"parts {parts}")
            total_val = 0
            for part in parts:
                # Remove 'stats.' from the beginning
                match = part[6:]  # Remove the first 6 characters ("stats.")
                if match in stats:
                    expr = expr.replace(f"stats.{match}", stats[match])
                    total_val = total_val + float(stats[match])
            expr = expr.replace(expr, str(stats[match]))

        else:
            # Remove 'stats.' from the beginning
            match = expr[6:]  # Remove the first 6 characters ("stats.")
            if match in stats:
                expr = expr.replace(f"stats.{match}", stats[match])
            else:
                expr = expr.replace(f"stats.{match}", "0")
                logging.warning(f"********* SECOND Stats key '{match}' not found. Defaulting to 0. expr {expr}")

    # Resolve config values
    #config_pattern = re.compile(r'config\.([\w\.]+)')
    #config_pattern = re.compile(r'config\.([\w\.]+(?:\.\w+)*)(?:::\w+)?')
    config_pattern = re.compile(r'config\.([\w\.]+(?:\.[\w\.]+)*)(?:::\w+)?')
    for match in config_pattern.findall(expr):
        conf_value = get_conf_value(match, config)
        expr = expr.replace(f"config.{match}", str(conf_value))

    if expr.startswith("config."):
        # Remove 'stats.' from the beginning
        match = expr[7:]  # Remove the first 6 characters ("stats.")
        if match in stats:
            expr = expr.replace(f"config.{match}", stats[match])
        else:
            expr = expr.replace(f"config.{match}", "0")
            logging.warning(f"********* SECOND config key '{match}' not found. Defaulting to 0. expr {expr}")

    # Now handle basic arithmetic expressions (e.g., 32*1 or 32*config.system.cpu_cluster.cpus.SThreads)
    #arithmetic_pattern = re.compile(r'(\d+)\*(\d+|\w+\.[\w\.]+)')
    arithmetic_pattern = re.compile(r'(\d+)\*(\d+|\w+\.[\w\.]+(?:::\S+)?)')
    while True:
        logging.debug(f"TEST2: {expr}")
        match = arithmetic_pattern.search(expr)
        logging.debug(f"TEST1: {expr}")
        if match:
            multiplier = int(match.group(1))  # The number before the '*' (32)
            operand = match.group(2)  # The operand after the '*' (either a number or config path)

            # Check if the operand is a config path or a number
            if operand.isdigit():  # If it's a number (e.g., "1")
                operand_value = int(operand)
            else:  # If it's a config path, resolve it
                operand_value = get_conf_value(operand, config)

            # Perform multiplication and replace the expression
            result = multiplier * operand_value
            expr = expr.replace(f"{match.group(0)}", str(result))  # Replace the whole expression (e.g., "32*1")

        else:
            break

    logging.debug(f"Resolved expression: {original_expr} -> {expr}")
    return expr


def process_template(template_file, stats, config):
    """Process the XML template, substituting stats and config values."""
    tree = ET.parse(template_file)
    root = tree.getroot()

    # Substitute stats and config values
    for param in root.iter('param'):
        value = param.attrib.get('value', '')
        param.attrib['value'] = evaluate_expression(value, stats, config)

    for stat in root.iter('stat'):
        value = stat.attrib.get('value', '')
        stat.attrib['value'] = evaluate_expression(value, stats, config)

    return tree

def main():
    parser = create_parser()
    args = parser.parse_args()

    logging.basicConfig(level=logging.DEBUG)  # Enable debug logging for troubleshooting

    # Read input files
    stats = read_stats_file(args.stats)
    config = read_config_file(args.config)

    # Process the XML template
    processed_tree = process_template(args.template, stats, config)

    # Write the processed XML to the output file
    with open(args.output, 'w') as output_file:
        output_file.write(prettify(processed_tree.getroot()))
    print(f"Processed XML written to {args.output}")

if __name__ == "__main__":
    main()
