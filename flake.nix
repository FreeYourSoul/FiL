{
  description = "Chaiscript library.";

  inputs.flake-utils.url = "github:numtide/flake-utils";

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let pkgs = nixpkgs.legacyPackages.${system}; in rec {

        defaultPackage = pkgs.stdenv.mkDerivation rec {
          version = "1.0.0";
          name = "fil-${version}";

          src = pkgs.fetchgit {
            url = "https://github.com/FreeYourSoul/Fil.git";
            rev = "18f69e32e5c576aed35c90f0a4ee468bf3418ce3";
	    sha256 = "sha256-XlglVmuTVOUESezE9CI8negiFkpQjFwxHqerJ4TiEnE=";
          };

          cmakeFlags =
            "\n -DBUILD_SERVICE_WORLD=OFF\n -DBUILD_SERVICE_QUEST=OFF\n -DBUILD_SERVICE_ARENA=OFF\n -DBUILD_SERVICE_INVENTORY=OFF\n -DBUILD_DISP_CHAT=OFF\n -DBUILD_DISP_GATEWAY=OFF\n -DBUILD_DISPATCHER=ON\n";

          buildInputs = with pkgs; [ cmake rocksdb catch2 fmt ];


        };

        fil = defaultPackage;

      });
}
