{ rev ? "c178d7e0625c87188ebd2b1bcd8cfc482376f10b", stdenv, cmake, rocksdb
, catch2, fmt }:

stdenv.mkDerivation rec {

  version = "1.0.1";
  name = "fil-${version}";

  src = builtins.fetchGit {
    url = "https://github.com/FreeYourSoul/Fil.git";
    rev = "${rev}";
  };

  buildInputs = [ cmake rocksdb catch2 fmt ];

}