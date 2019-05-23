######################################
#
# mod-classic-midi-sequencer
#
######################################

MOD_CLASSIC_MIDI_SEQUENCER_VERSION = 092db5ad71d8f1b22f950466f5b3517309d32499
MOD_CLASSIC_MIDI_SEQUENCER_SITE = $(call github,moddevices,mod-classic-midi-sequencer,$(MOD_CLASSIC_MIDI_SEQUENCER_VERSION))
MOD_CLASSIC_MIDI_SEQUENCER_BUNDLES = Step-Sequencer.lv2


# dependencies (list of other buildroot packages, separated by space)
MOD_CLASSIC_MIDI_SEQUENCER_DEPENDENCIES =

# call make with the current arguments and path. "$(@D)" is the build directory.
MOD_CLASSIC_MIDI_SEQUENCER_TARGET_MAKE = $(TARGET_MAKE_ENV) $(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D)/step-sequencer/source


# build command
define MOD_CLASSIC_MIDI_SEQUENCER_BUILD_CMDS
	$(MOD_CLASSIC_MIDI_SEQUENCER_TARGET_MAKE)
endef

# install command
define MOD_CLASSIC_MIDI_SEQUENCER_INSTALL_TARGET_CMDS
	$(MOD_CLASSIC_MIDI_SEQUENCER_TARGET_MAKE) install DESTDIR=$(TARGET_DIR)
endef


# import everything else from the buildroot generic package
$(eval $(generic-package))
