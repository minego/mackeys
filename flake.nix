{
	description			= "A flake for building my mackeys plugin for interception-tools";

	inputs = {
		nixpkgs.url			= "github:NixOS/nixpkgs/nixos-unstable";
		flake-utils. url	= "github:numtide/flake-utils";
	};

	outputs = { self, nixpkgs, flake-utils }: flake-utils.lib.eachSystem [ "x86_64-linux" "aarch64-linux" ]  (system:
		let pkgs = nixpkgs.legacyPackages.${system}; in
	{
		packages = rec {
			mackeys = pkgs.callPackage ./derivation.nix { };
			default = mackeys;
		};

		overlay = self: super: {
			minego = (super.minego or {}) // {
				mackeys = super.pkgs.callPackage ./derivation.nix { };
			};
		};
	});
}


