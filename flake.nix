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
            rev = "f76863ade50f3d0ab837c26a3df72d6a311ce3eb";
	    sha256 = "sha256-s7XZJxrjnMkIlUwWRGOOPOp0yYyYNOl+xI6VhU1xkD0=";
          };

          cmakeFlags =
            "\n -DBUILD_SERVICE_WORLD=OFF\n -DBUILD_SERVICE_QUEST=OFF\n -DBUILD_SERVICE_ARENA=OFF\n -DBUILD_SERVICE_INVENTORY=OFF\n -DBUILD_DISP_CHAT=OFF\n -DBUILD_DISP_GATEWAY=OFF\n -DBUILD_DISPATCHER=ON\n";

          buildInputs = with pkgs; [ rocksdb catch2 fmt ];

	  nativeBuildInputs = with pkgs; [ cmake pkgconfig ];

        };

        fil = defaultPackage;

      });
}
