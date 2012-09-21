update-pot:
	../utils/make-pot.sh > frogatto.pot.bak
	msguniq -F frogatto.pot.bak > frogatto.pot
	rm frogatto.pot.bak

%.po: frogatto.pot
	msgmerge $@ frogatto.pot -o $@.part
	mv $@.part $@

LINGUAS=ar cs de el en_GB eo es fr gd hu_HU id it ja ms_MY nl pl pt_BR ru sk sv tt zh_CN

update-po:
	(for lang in ${LINGUAS}; do \
		${MAKE} $$lang.po ; \
	done)

update-mo:
	(for lang in ${LINGUAS}; do \
		mkdir -p ../locale/$$lang/LC_MESSAGES ; \
		msgfmt $$lang.po -o ../locale/$$lang/LC_MESSAGES/frogatto.mo ; \
	done)

