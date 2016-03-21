update-pot:
	../utils/make-pot.sh > frogatto.pot.bak
	msguniq -F frogatto.pot.bak > frogatto.pot
	rm frogatto.pot.bak

%.po: frogatto.pot
	msgmerge $@ frogatto.pot -o $@.part
	mv $@.part $@

LINGUAS=ar be_BY br ca_ES cs da de el eo es fr gd he hu_HU id it ja ko ms_MY mt nl pl pt_BR pt_PT ru_RU sk sr@latin sv tt zh_CN

update-po:
	(for lang in ${LINGUAS}; do \
		mkdir -p ../locale/$$lang/LC_MESSAGES ; \
		${MAKE} ../locale/$$lang/LC_MESSAGES/$$lang.po ; \
	done)


