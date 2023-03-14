{
  description = "Fil : A C++ generic library";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";
  };


  outputs = { self, nixpkgs, flake-utils }: {
    overlay = final: prev: {
      fil = with final; stdenv.mkDerivation {
        pname = "fil";
        version = "1.0.1";

	    src = builtins.fetchGit {
          url = "https://github.com/FreeYourSoul/Fil.git";
          rev = "c178d7e0625c87188ebd2b1bcd8cfc482376f10b";
        };

        buildInputs = [ cmake rocksdb catch2 fmt ];
        };
    };
  } //
  flake-utils.lib.eachDefaultSystem (system: {
    defaultPackage = (import nixpkgs {
      inherit system;
      overlays = [ self.overlay ];
    }).fil;
  });


}
