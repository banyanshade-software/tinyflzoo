###########################################################################
##                                                                       ##
##                                OCaPIC                                 ##
##                                                                       ##
##                             Benoit Vaugon                             ##
##                                                                       ##
##    This file is distributed under the terms of the CeCILL license.    ##
##    See file ../../../LICENSE-en.                                      ##
##                                                                       ##
###########################################################################

include ../../../etc/Makefile.conf

TARG = $(BIN)/ocapic_dip40_simulator

all: $(TARG)

$(TARG): main.ml
	$(OCAMLOPT) -thread -I $(LIB)/extra -I +../graphics unix.cmxa threads.cmxa graphics.cmxa simul.cmx $< -o $@

clean:
	@rm -f *~ *.o *.cmi *.cmx $(TARG)

.PHONY: all clean
