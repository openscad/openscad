EXAMPLES := ../../examples
SRC=$(wildcard $(EXAMPLES)/*/*.scad)
PNG=$(patsubst $(EXAMPLES)/%.scad,html/%.png,$(SRC))
HTML=$(patsubst $(EXAMPLES)/%.scad,html/%.html,$(SRC))

OPENSCAD := ../../OpenSCAD.app/Contents/MacOS/OpenSCAD
#OPENSCAD := openscad
ARGS := --imgsize=1200,900 --camera=0,0,0,55,0,25,180 --viewall --autocenter

all : $(PNG) $(HTML) example-data.js assets

.PHONY: clean
clean :
	rm -rf html

.PHONY: assets
assets :
	cp -a assets html/

.PHONY: example-data.js
example-data.js :
	( \
	echo "openscad_examples = ["; \
	for a in Basics Functions Shapes Extrusion Advanced; \
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
	$(OPENSCAD) $(ARGS) -o $@ $< > out.log 2>&1

html/%.html : $(EXAMPLES)/%.scad template-pre.html template-post.html
	#echo $(notdir $(patsubst %.scad,%.png,$<))
	mkdir -p `dirname $@`
	sed -e 's/@@IMAGE@@/$(notdir $(patsubst %.scad,%.png,$<))/g' template-pre.html > $@
	sed -e 's/</\&lt/; s/>/\&gt/;' $< >> $@
	cat template-post.html >> $@
