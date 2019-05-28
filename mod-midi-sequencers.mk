######################################
#
# mod-midi-sequencers
#
######################################

MOD_MIDI_SEQUENCERS_VERSION = 973189b6be8b150b3094a8b7d1e0df39f3c93236
MOD_MIDI_SEQUENCERS_SITE = $(call github,moddevices,mod-classic-midi-sequencer,$(MOD_MIDI_SEQUENCERS_VERSION))
MOD_MIDI_SEQUENCERS_BUNDLES = Step-Sequencer.lv2


# dependencies (list of other buildroot packages, separated by space)
MOD_MIDI_SEQUENCERS_DEPENDENCIES = lv2

# call make with the current arguments and path. "$(@D)" is the build directory.
MOD_MIDI_SEQUENCERS_TARGET_MAKE = $(TARGET_MAKE_ENV) $(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D)/step-sequencer/source


# build command
define MOD_MIDI_SEQUENCERS_BUILD_CMDS
	$(MOD_MIDI_SEQUENCERS_TARGET_MAKE)
endef

# install command
define MOD_MIDI_SEQUENCERS_INSTALL_TARGET_CMDS
	$(MOD_MIDI_SEQUENCERS_TARGET_MAKE) install DESTDIR=$(TARGET_DIR)
endef


# import everything else from the buildroot generic package
$(eval $(generic-package))
