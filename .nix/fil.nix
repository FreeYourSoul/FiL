{ lib
, stdenv
, cmake
, ninja
, gcc
, catch2_3
, fmt
, rocksdb
, execute_test ? false
}:
let
    version = builtins.readFile ../VERSION;
in
stdenv.mkDerivation rec {
  pname = "fil";
  inherit version;

  src = ../.;

  nativeBuildInputs = [
    cmake
    ninja
    gcc
  ];

  buildInputs = [
    catch2_3
    fmt
    rocksdb
  ];


  cmakeFlags = [
    "-DBUILD_TESTING=${if execute_test then "ON" else "OFF"}"
    "-DCMAKE_INSTALL_PREFIX=${placeholder "out"}"
    "-DCMAKE_INSTALL_INCLUDEDIR=${placeholder "out"}/include"
    "-DCMAKE_INSTALL_LIBDIR=${placeholder "out"}/lib"
  ];

  doCheck = execute_test;

  postInstall = ''
    substituteInPlace $out/lib/cmake/fil/filTargets.cmake --replace "/build/FiL/;" ""
    '';

  meta = with lib; {
    description = "Header-only C++ Utility library";
    homepage = "https://github.com/FreeYourSoul/FiL";
    license = licenses.agpl3Plus;
    platforms = platforms.all;
    maintainers = with maintainers; [
      freeyoursoul
    ];
  };
}
