#! /usr/bin/python

import argparse
import os
import re


def get_parser():
    parser = argparse.ArgumentParser(description='Compile dicts for hunspell')
    parser.add_argument('--categories', help='path to categories file', required=True)
    parser.add_argument('--dicts', help='path to docts folder', required=True)
    parser.add_argument('--output', help='path to output folder', required=True)
    return parser

def get_languages(categories_file_path):
    languages = set()
    with open(categories_file_path, 'r') as f:
        for line in f:
            m = re.match(r'^([a-z]{2}):', line)
            if m:
                languages.add(m.group(1))
    return languages

def get_encoding(affixes_string):
    m = re.search(r'SET(?:\s+)([^\r\n]+)', affixes_string)
    return m.group(1).strip()

def change_encoding_srting(s, from_enc):
    if from_enc.upper() in ['UTF8', 'UTF-8']:
        return s
    if from_enc.upper() == 'TIS620-2533':
        from_enc = 'ISO-IR-166'
    return s.decode(from_enc).encode('UTF-8')

def replace_set_encoding(aff, enc):
    return re.sub(r'SET(\s+){}'.format(enc), 'SET UTF-8', aff)

def filter_dicts_paths(dicts_path, languages):
    dicts = filter(lambda x: x.endswith('.aff') or x.endswith('.dic'),
                   os.listdir(dicts_path))
    filtered_dicts = filter(lambda x: x[:2] in languages, dicts)
    affs = sorted(filter(lambda x: x.endswith('.aff'), filtered_dicts))
    dics = sorted(filter(lambda x: x.endswith('.dic'), filtered_dicts))
    return zip(affs, dics)

def read_file(path):
    with open(path, 'r') as f:
        contents = f.read()
    return contents

def write_file(contents, path):
    with open(path, 'w') as f:
        f.write(contents)

def compile_dicts(dicts_path, output_path, languages):
    for aff, dic in filter_dicts_paths(dicts_path, languages):
        aff_contents = read_file(os.path.join(dicts_path, aff))
        dic_contents = read_file(os.path.join(dicts_path, dic))

        enc = get_encoding(aff_contents)
        print 'processing {}, {} with enc = {}'.format(aff, dic, enc)

        aff_contents = replace_set_encoding(aff_contents, enc)
        aff_contents = change_encoding_srting(aff_contents, enc)
        dic_contents = change_encoding_srting(dic_contents, enc)

        write_file(aff_contents, os.path.join(output_path, aff))
        write_file(dic_contents, os.path.join(output_path, dic))

if __name__ == '__main__':
    parser = get_parser()
    args = parser.parse_args()
    languages = get_languages(args.categories)
    compile_dicts(args.dicts, args.output, languages)
