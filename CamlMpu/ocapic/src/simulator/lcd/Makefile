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

TARG = $(BIN)/ocapic_lcd_simulator
SRCS = $(wildcard *.ml)
BUILD = main.native
EXTRA = $(LIB)/extra
CFLAGS = -thread,-I,$(EXTRA),-I,+../graphics
LFLAGS = $(CFLAGS),unix.cmxa,graphics.cmxa,threads.cmxa,simul.cmx
OPTIONS = -cflags $(CFLAGS) -lflags $(LFLAGS)

simu_lcd: $(TARG)

$(TARG): _build/$(BUILD)
	cp $< $@

_build/$(BUILD): $(SRCS) config.ml $(EXTRA)/simul.ml $(EXTRA)/simul.mli
	$(OCAMLBUILD) $(OPTIONS) $(BUILD)
	@touch $@

config.ml: $(ETC)/config.ml
	cp $< $@

clean:
	@$(OCAMLBUILD) -clean
	@rm -f *~ config.ml $(TARG)

.PHONY: simu_lcd clean
