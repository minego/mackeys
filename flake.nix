{
	description			= "A flake for building my mackeys plugin for interception-tools";
	inputs.nixpkgs.url	= "github:NixOS/nixpkgs/nixos-unstable";

	outputs = { self, nixpkgs, ... }:
	{
		overlay = self: super: {
			minego = (super.minego or {}) // {
				mackeys = super.pkgs.callPackage ./derivation.nix { };
			};
		};

		packages.x86_64-linux.mackeys = nixpkgs.legacyPackages.x86_64-linux.callPackage ./derivation.nix {};
		packages.x86_64-linux.default = self.packages.x86_64-linux.mackeys;

		packages.aarch64-linux.mackeys = nixpkgs.legacyPackages.aarch64-linux.callPackage ./derivation.nix {};
		packages.aarch64-linux.default = self.packages.aarch64-linux.mackeys;
	};
}


