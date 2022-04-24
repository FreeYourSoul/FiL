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
            rev = "ebdb5883b81e1b3764e43bcc79b4e72f3db03f32";
	    sha256 = "sha256-s4qhNVtWsqs41QlvWkh9906nAkBkChkqvSBXd4Fql3U=";
          };

          cmakeFlags =
            "\n -DBUILD_SERVICE_WORLD=OFF\n -DBUILD_SERVICE_QUEST=OFF\n -DBUILD_SERVICE_ARENA=OFF\n -DBUILD_SERVICE_INVENTORY=OFF\n -DBUILD_DISP_CHAT=OFF\n -DBUILD_DISP_GATEWAY=OFF\n -DBUILD_DISPATCHER=ON\n";

          buildInputs = with pkgs; [ cmake rocksdb catch2 fmt ];


        };

        fil = defaultPackage;

      });
}
