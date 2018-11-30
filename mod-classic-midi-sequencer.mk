######################################
#
# mod-classic-midi-sequencer
#
######################################

# where to find the source code - locally in this case
MOD_CLASSIC_MIDI_SEQUENCER_SITE_METHOD = local
MOD_CLASSIC_MIDI_SEQUENCER_SITE = $($(PKG)_PKGDIR)/

# even though this is a local build, we still need a version number
# bump this number if you need to force a rebuild
MOD_CLASSIC_MIDI_SEQUENCER_VERSION = 0.1

# dependencies (list of other buildroot packages, separated by space)
MOD_CLASSIC_MIDI_SEQUENCER_DEPENDENCIES =

# LV2 bundles that this package generates (space separated list)
MOD_CLASSIC_MIDI_SEQUENCER_BUNDLES = Classic-MIDI-Sequencer.lv2

# call make with the current arguments and path. "$(@D)" is the build directory.
MOD_CLASSIC_MIDI_SEQUENCER_TARGET_MAKE = $(TARGET_MAKE_ENV) $(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D)/source


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
