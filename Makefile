SELF := proput
VERSION := $(shell git describe | sed 's/^v//')
DEB_NAME := $(SELF)-$(VERSION).deb
DEB_DIR := build/$(SELF)-$(VERSION)

MKDIR = mkdir -p $(dir $@)

deb: $(addprefix $(DEB_DIR)/debian/,control changelog rules)
	cd $(DEB_DIR) && dpkg-buildpackage -uc -us -rfakeroot

%/control: debian/control.gen
	$(MKDIR)
	$< $(SELF) $(VERSION) > $@

%/changelog: Makefile
	$(MKDIR)
	rm -f $@
	dch --changelog $@ --create --package=$(SELF) --newversion=$(VERSION) "Automatic release."
	sed -i '/Initial release/d' $@

$(DEB_DIR)/debian/rules: debian/rules
	$(MKDIR)
	cp $< $@

clean::
	rm -rf $(DEB_DIR) build

.PHONY: deb clean
