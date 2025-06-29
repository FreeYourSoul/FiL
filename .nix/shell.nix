{ pkgs ? import <nixpkgs> {} }:

with pkgs;

let
  fil = pkgs.callPackage ./fil.nix {};
in
mkShell {
  buildInputs = [
    cmake
    gcc
    ninja
    gdb
    lcov
    gcovr
  ] ++ fil.buildInputs;

  shellHook = ''
    echo "C++ development environment loaded"
  '';
}
