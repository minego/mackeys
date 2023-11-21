{
	description = "A flake for building my mackeys plugin for interception-tools";

	inputs = {
		nixpkgs.url		= "github:NixOS/nixpkgs/nixos-unstable";
	};

	outputs = { self, nixpkgs, ... }:
	{
		packages.x86_64-linux.default = 
			with import nixpkgs { system = "x86_64-linux"; };

			stdenv.mkDerivation rec {
			  name = "mackeys";
			  version = "0.0.1";

			  src = fetchFromGitHub {
				owner = "minego";
				repo = "mackeys";
				rev = "104b2225ebeac6237502547b74ce8b6d7fa83ea9"; # Feb 10, 2022
				sha256 = "sha256-0gulm0RfOGGGDyL6aYTG6FtLpARbpQ8fNJth4hqjfyY=";
			  };

			  installPhase = ''
				mkdir -p $out/bin
				cp mackeys $out/bin/
			  '';

			  meta = with lib; {
				homepage = "https://github.com/minego/mackeys";
				description = "Allow using super with x,c,v for consistent copy and paste everywhere";
				license = licenses.asl20; # Apache-2.0
				platforms = platforms.linux;
			  };
			};
	};
}