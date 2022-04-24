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
            rev = "28a8257267c8339e66eeb6eccaa0823114fd4020";
	    sha256 = "sha256-5LkCFZeqJVz0SBd0SUjGewSm8qcQK1kf8Km/vfMC2fg=";
          };

          cmakeFlags =
            "\n -DBUILD_SERVICE_WORLD=OFF\n -DBUILD_SERVICE_QUEST=OFF\n -DBUILD_SERVICE_ARENA=OFF\n -DBUILD_SERVICE_INVENTORY=OFF\n -DBUILD_DISP_CHAT=OFF\n -DBUILD_DISP_GATEWAY=OFF\n -DBUILD_DISPATCHER=ON\n";

          buildInputs = with pkgs; [ rocksdb catch2 fmt ];

	  nativeBuildInputs = with pkgs; [ cmake pkgconfig ];

        };

        fil = defaultPackage;

      });
}
