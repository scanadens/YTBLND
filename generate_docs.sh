#!/bin/bash
# script to generate documentation based on doxygen comments through the project

# Check if pdflatex is installed
if ! command -v pdflatex &> /dev/null; then
    echo "Error: pdflatex is not installed."
    echo ""
    echo "Please install the required LaTeX packages:"
    echo ""
    echo "On Ubuntu/Debian:"
    echo "  sudo apt install texlive-latex-base texlive-latex-extra texlive-fonts-recommended"
    echo ""
    echo "On Fedora/RHEL:"
    echo "  sudo dnf install texlive-collection-latexextra texlive-collection-fontsrecommended"
    echo ""
    echo "On Arch:"
    echo "  sudo pacman -S texlive-latex texlive-latex-extra"
    echo ""
    exit 1
fi

# Generate HTML docs (already exists)
doxygen Doxyfile

# Generate PDF docs
echo "Generating PDF documentation..."
make -C build/doxygen_output/latex pdf

if [ $? -eq 0 ]; then
    echo "PDF generated: build/doxygen_output/latex/refman.pdf"
    cp build/doxygen_output/latex/refman.pdf Docs/
else
    echo "PDF generation failed. Check build/doxygen_output/latex/refman.log for details."
    exit 1
fi
