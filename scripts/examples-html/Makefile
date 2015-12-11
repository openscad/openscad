EXAMPLES := ../../examples
LOG := out.log
SRC=$(wildcard $(EXAMPLES)/*/*.scad)
PNG=$(patsubst $(EXAMPLES)/%.scad,html/%.png,$(SRC))
HTML=$(patsubst $(EXAMPLES)/%.scad,html/%.html,$(SRC))

OPENSCAD := $(shell ls ../../openscad ../../OpenSCAD.app/Contents/MacOS/OpenSCAD 2>/dev/null | head -n 1)
ARGS := --imgsize=1200,900 --camera=0,0,0,55,0,25,180 --viewall --autocenter

all : info $(PNG) $(HTML) example-data.js assets

.PHONY: info
info:
	@echo Using $(OPENSCAD)

.PHONY: clean
clean :
	rm -rf html $(LOG)

.PHONY: assets
assets :
	cp -a assets html/

.PHONY: example-data.js
example-data.js :
	( \
	echo "openscad_examples = ["; \
	for a in `grep '\[' ../../examples/examples.json | tr -d ' \t":[]'`; \
	do \
		echo "  {"; \
		echo "    name : \"$$a\","; \
		echo "    files : ["; \
		for f in "$(EXAMPLES)/$$a/"*.scad ; \
		do \
			echo "      \"`basename -s .scad $$f`\","; \
		done; \
		echo "    ]"; \
		echo "  },"; \
	done; \
	echo "];" \
	) > html/example-data.js

html/%.png : $(EXAMPLES)/%.scad
	mkdir -p `dirname $@`
	$(OPENSCAD) $(ARGS) -o $@ $< > $(LOG) 2>&1

html/%.html : $(EXAMPLES)/%.scad template-pre.html template-post.html
	#echo $(notdir $(patsubst %.scad,%.png,$<))
	mkdir -p `dirname $@`
	sed -e 's/@@IMAGE@@/$(notdir $(patsubst %.scad,%.png,$<))/g' template-pre.html > $@
	sed -e 's/</\&lt/; s/>/\&gt/;' $< >> $@
	cat template-post.html >> $@
