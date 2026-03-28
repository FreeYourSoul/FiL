{
  description = "Fil: Header-only C++ Utility library";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        
        fil = pkgs.callPackage ./.nix/fil.nix { };
        fil-test = pkgs.callPackage ./.nix/fil.nix {
            execute_test = true;
            with_coverage = true;
        };
      in
      {
        packages.default = fil;
        packages.fil = fil;
        packages.fil-test = fil-test;

        devShells.default = pkgs.mkShell {
          inputsFrom = [ fil ];
          nativeBuildInputs = with pkgs; [
            cmake
            ninja
            gcc
            gdb
            lcov
            gcovr
          ];
          shellHook = ''
            echo "Fil C++ development environment loaded (via Flake)"
          '';
        };
      }
    );
}
