all: pdf

pdf: jtdutils.tex build.tex
	pdflatex -interaction=nonstopmode jtdutils.tex

html: jtdutils.tex
	mkdir -p html
	latex2html jtdutils.tex -local_icons -dir ./html
