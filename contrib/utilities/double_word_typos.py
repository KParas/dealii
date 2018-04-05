#!/usr/bin/env python3
# ---------------------------------------------------------------------
#
# Copyright (C) 2018 by the deal.II authors
#
# This file is part of the deal.II library.
#
# The deal.II library is free software; you can use it, redistribute
# it, and/or modify it under the terms of the GNU Lesser General
# Public License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
# The full text of the license can be found in the file LICENSE at
# the top level of the deal.II distribution.
#
# ---------------------------------------------------------------------
"""A script that locates double word typos, e.g., from tria.h:

    This way it it possible to obtain neighbors across a periodic

This script should be run as

   python double_word_typos.py /path/to/header.h

or, in a loop, e.g.,

    for FILE in $(find ./examples/ -name "*.dox")
    do
        python ./contrib/utilities/double_word_typos.py $FILE
    done

and will print lines containing double words (or double lines, if the first word
of the next line matches the last word of the previous line) to the screen.

The output format used by this script is similar to GCC's and it can be run very
conveniently from inside emacs' compilation-mode.
"""
import sys

# Skip the following tokens since they show up frequently but are not related to
# double word typos
SKIP = ["//", "*", "}", "|", "};", ">", "\"", "|", "/",
        "numbers::invalid_unsigned_int,", "std::string,", "int,"]

with open(sys.argv[1], 'r') as handle:
    previous_line = ""
    for line_n, line in enumerate(handle):
        line = line.strip()
        previous_words = previous_line.split()
        words = line.split()
        # ignore comment blocks '*' and comment starts '//' at the beginning of
        # each line.
        if len(words) == 0:
            continue
        if words[0] in ["*", "//"]:
            words = words[1:]

        # See if the last word on the previous line is equal to the first word
        # on the current line.
        if len(previous_words) != 0:
            if words[0] not in SKIP and previous_words[-1] == words[0]:
                print(sys.argv[1] + ":{}: {}".format(line_n + 1, previous_line))
                print(sys.argv[1] + ":{}: {}".format(line_n + 2, line))
        previous_line = line

        for left_word, right_word in zip(words[:-1], words[1:]):
            if left_word == right_word and left_word not in SKIP:
                print(sys.argv[1] + ":{}: {}".format(line_n + 1, line))
