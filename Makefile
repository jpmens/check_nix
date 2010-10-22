all: check_nix.8 README.md

check_nix.8: check_nix.pandoc
	pandoc -s -w man check_nix.pandoc -o check_nix.8

README.md: check_nix.pandoc
	pandoc -w markdown check_nix.pandoc -o README.md
