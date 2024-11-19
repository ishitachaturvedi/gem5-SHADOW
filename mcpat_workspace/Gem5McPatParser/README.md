# Gem5 to McPAT parser 
This script converts Gem5 simulation statistics to McPAT compatible input files. It supports multiple cores as well as multiple private or one shared L2 cache.

### Gem5 Supported Version

Last tested on Gem5 version (tag: v20.1.0.2) stable. You can fetch this version using:

```sh
# assuming you have git installed
git clone https://gem5.googlesource.com/public/gem5

# go into cloned gem5 repo
cd gem5

# checkout supported commit using hash
git checkout 0d703041fcd5d119012b62287695723a2955b408
```

### McPat Supported Version

This script was tested with McPat v1.3.0. This can be attained from 

```sh
# clone mcpat
git clone https://github.com/HewlettPackard/mcpat

# change directory to mcpat
cd mcpat

# go to last tested version tag
git checkout tags/v1.3.0
```


### Usage

Parser requires [python 3.6](https://www.python.org/download/releases/3.6/) to run.

```sh
 usage: Gem5McPATParser.py [-h] --config PATH --stats PATH --template PATH
                                    [--output PATH]

        Gem5 to McPAT parser

        optional arguments:
        -h, --help            show this help message and exit
        --config PATH, -c PATH
                                Input config.json from Gem5 output.
        --stats PATH, -s PATH
                                Input stats.txt from Gem5 output.
        --template PATH, -t PATH
                                Template XML file
        --output PATH, -o PATH
                                Output file for McPAT input in XML format (default:
                                mcpat-in.xml)
```

### Example

```sh
$ python3 Gem5McPATParser.py -c config.json -s stats.txt -t template.xml
```

### TODO

- test multicore support




