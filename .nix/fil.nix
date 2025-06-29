{ lib
, stdenv
, cmake
, ninja
, gcc
, catch2_3
, fmt
, rocksdb
, lcov
, gcovr
, execute_test ? false
, with_coverage ? false
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
  ] ++ lib.optionals execute_test [
    lcov
    gcovr
  ];

  buildInputs = [
    catch2_3
    fmt
    rocksdb
  ];

  cmakeFlags = [
    "-DBUILD_TESTING=${if with_coverage then "ON" else "OFF"}"
    "-DCMAKE_INSTALL_PREFIX=${placeholder "out"}"
    "-DCMAKE_INSTALL_INCLUDEDIR=${placeholder "out"}/include"
    "-DCMAKE_INSTALL_LIBDIR=${placeholder "out"}/lib"
  ];

  CXXFLAGS = lib.optionalString with_coverage "-coverage -O0 -g";

  doCheck = execute_test;

  postCheck = lib.optionalString with_coverage ''
    echo "Generating coverage report... ${src}"
    ctest -T Coverage

    mkdir -p $out/coverage
    find -name "*.xml" -exec cp -r {} $out/coverage/ \ ;

  '';

  postInstall = ''
    substituteInPlace $out/lib/cmake/fil/filTargets.cmake --replace-warn "/build/FiL/;" ""
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
