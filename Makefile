all:
	$(MAKE) -C step-sequencer/source
	$(MAKE) -C phrase-sequencer/source
clean:
	$(MAKE) clean -C step-sequencer/source
	$(MAKE) clean -C phrase-sequencer/source
