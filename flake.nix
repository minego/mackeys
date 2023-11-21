{
	description			= "A flake for building my mackeys plugin for interception-tools";
	inputs.nixpkgs.url	= "github:NixOS/nixpkgs/nixos-unstable";

	outputs = { self, nixpkgs, ... }:
	{
		packages.x86_64-linux.default = 
			with import nixpkgs { system = "x86_64-linux"; };

		stdenv.mkDerivation rec {
			name		= "mackeys";
			version		= "0.0.1";

			src			= self;

			installPhase = ''
				mkdir -p $out/bin
				cp mackeys $out/bin/
			'';

			meta = with lib; {
				homepage	= "https://github.com/minego/mackeys";
				description	= "Allow using super with x,c,v for consistent copy and paste everywhere";
				license		= licenses.asl20; # Apache-2.0
				platforms	= platforms.linux;
			};
		};
	};
}
