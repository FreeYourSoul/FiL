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
            rev = "c178d7e0625c87188ebd2b1bcd8cfc482376f10b";
	    sha256 = "sha256-t0plapDg5y29fH696HT7mF7n9YKm2r3/vezV14PG4vI=";
          };

          cmakeFlags =
            "\n -DBUILD_SERVICE_WORLD=OFF\n -DBUILD_SERVICE_QUEST=OFF\n -DBUILD_SERVICE_ARENA=OFF\n -DBUILD_SERVICE_INVENTORY=OFF\n -DBUILD_DISP_CHAT=OFF\n -DBUILD_DISP_GATEWAY=OFF\n -DBUILD_DISPATCHER=ON\n";

          buildInputs = with pkgs; [ cmake rocksdb catch2 fmt ];
        };

        fil = defaultPackage;

      });
}
