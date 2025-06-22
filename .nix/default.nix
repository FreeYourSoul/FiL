{ pkgs ? import <nixpkgs> {} }:

let
  fil = pkgs.callPackage ./fil.nix {};
in
  fil
