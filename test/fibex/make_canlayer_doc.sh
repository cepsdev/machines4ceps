#!/bin/sh

../../x86/sm prelude.ceps $1.xml --no_warn --dump_asciidoc_can_layer > $1.adoc
asciidoctor $1.adoc

