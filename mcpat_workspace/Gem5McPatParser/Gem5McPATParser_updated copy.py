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
    """Resolve a key path in the config dictionary."""
    logging.debug(f"Resolving config path: {conf_str}")
    keys = conf_str.split('.')
    value = config
    for key in keys:
        if isinstance(value, list):
            try:
                key = int(key)  # Convert to int for list indexing
                value = value[key]
            except (ValueError, IndexError):
                raise KeyError(f"Key '{key}' not found in list at '{conf_str}'")
        elif isinstance(value, dict) and key in value:
            value = value[key]
        else:
            raise KeyError(f"Key '{key}' not found in config path '{conf_str}'")
    logging.debug(f"Resolved config path '{conf_str}' to value: {value}")
    return value


def evaluate_expression(expr, stats, config):
    """Evaluate a stat or config expression."""
    original_expr = expr
    logging.debug(f"Evaluating expression: {original_expr}")

    # Resolve stats
    stat_pattern = re.compile(r'stats\.([\w\.]+)')
    for match in stat_pattern.findall(expr):
        if match in stats:
            expr = expr.replace(f"stats.{match}", stats[match])
        else:
            logging.warning(f"Stats key '{match}' not found. Defaulting to 0.")
            expr = expr.replace(f"stats.{match}", "0")

    # Resolve config
    config_pattern = re.compile(r'config\.([\w\.]+)')
    for match in config_pattern.findall(expr):
        try:
            conf_value = get_conf_value(match, config)
            expr = expr.replace(f"config.{match}", str(conf_value))
        except KeyError:
            logging.warning(f"Config key '{match}' not found. Defaulting to 0.")
            expr = expr.replace(f"config.{match}", "0")

    # Evaluate the final expression
    try:
        resolved_value = str(eval(expr))
        logging.debug(f"Resolved expression: {original_expr} -> {resolved_value}")
        return resolved_value
    except Exception as e:
        logging.error(f"Failed to evaluate expression '{expr}': {e}")
        return "0"


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
