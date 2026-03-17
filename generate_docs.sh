#!/bin/bash
# script to generate documentation based on doxygen comments through the project

make -C build/doxygen_output/latex pdf
cp build/doxygen_output/latex/refman.pdf Docs/
echo "PDF of code documentation generated and copied to /Docs/ as refman.pdf"