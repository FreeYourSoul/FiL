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
  ] ++ fil.buildInputs;

  shellHook = ''
    echo "C++ development environment loaded"
  '';
}
