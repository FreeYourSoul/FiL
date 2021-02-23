{ rev ? "c178d7e0625c87188ebd2b1bcd8cfc482376f10b", stdenv, cmake, rocksdb
, catch2, fmt }:

stdenv.mkDerivation rec {

  version = "1.0.0";
  name = "fil-${version}";

  src = builtins.fetchGit {
    url = "https://github.com/FreeYourSoul/Fil.git";
    rev = "${rev}";
  };

  cmakeFlags =
    "\n    -DBUILD_SERVICE_WORLD=OFF\n    -DBUILD_SERVICE_QUEST=OFF\n    -DBUILD_SERVICE_ARENA=OFF\n    -DBUILD_SERVICE_INVENTORY=OFF\n    -DBUILD_DISP_CHAT=OFF\n    -DBUILD_DISP_GATEWAY=OFF\n    -DBUILD_DISPATCHER=ON\n    ";

  buildInputs = [ cmake rocksdb catch2 fmt ];

}
