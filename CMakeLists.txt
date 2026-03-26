# ============================================================================
# CMakeLists.txt — Configuração de build principal
#
# CMake é o sistema de build padrão em projetos C++ profissionais.
# Ele gera Makefiles (Linux), projetos Visual Studio (Windows), etc.
#
# Para compilar:
#   mkdir build && cd build
#   cmake ..
#   make
# ============================================================================

cmake_minimum_required(VERSION 3.16)
project(SimianDetector VERSION 1.0.0 LANGUAGES CXX)

# C++17 — usamos features como std::string_view, structured bindings, etc.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# Dependências externas
# ============================================================================

# PostgreSQL (via libpq)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQ REQUIRED libpq)

# OpenSSL (para SHA-256 no hash do DNA)
find_package(OpenSSL REQUIRED)

# Threads (cpp-httplib precisa de pthreads)
find_package(Threads REQUIRED)

# ============================================================================
# Biblioteca Core (a lógica de detecção)
# Compilada como lib separada para ser reutilizada nos testes e na API
# ============================================================================
add_library(simian_core STATIC
    src/core/simian_detector.cpp
)
target_include_directories(simian_core PUBLIC src/core)

# ============================================================================
# Biblioteca Database
# ============================================================================
add_library(simian_db STATIC
    src/db/database.cpp
)
target_include_directories(simian_db PUBLIC src/db)
target_link_libraries(simian_db
    ${LIBPQ_LIBRARIES}
    OpenSSL::Crypto
)
target_include_directories(simian_db PUBLIC ${LIBPQ_INCLUDE_DIRS})

# ============================================================================
# Executável da API
# ============================================================================
add_executable(simian_api
    src/main.cpp
    src/api/server.cpp
)
target_include_directories(simian_api PRIVATE
    src
    third_party  # cpp-httplib
)
target_link_libraries(simian_api
    simian_core
    simian_db
    Threads::Threads
    ${LIBPQ_LIBRARIES}
    OpenSSL::Crypto
)

# ============================================================================
# Testes (Google Test)
# ============================================================================
# Para habilitar testes: cmake -DBUILD_TESTS=ON ..
option(BUILD_TESTS "Build unit tests" ON)

if(BUILD_TESTS)
    enable_testing()
    find_package(GTest QUIET)

    if(GTest_FOUND)
        add_executable(simian_tests tests/test_simian.cpp)
        target_include_directories(simian_tests PRIVATE src)
        target_link_libraries(simian_tests
            simian_core
            GTest::GTest
            GTest::Main
        )
        add_test(NAME SimianTests COMMAND simian_tests)
        message(STATUS "Tests enabled — run with: ctest or ./simian_tests")
    else()
        message(STATUS "Google Test not found. Install with: sudo apt install libgtest-dev")
    endif()
endif()

# ============================================================================
# Frontend Qt Quick/QML (compilação opcional)
# Para habilitar: cmake -DBUILD_QT_FRONTEND=ON ..
# ============================================================================
option(BUILD_QT_FRONTEND "Build Qt Quick/QML frontend application" OFF)

if(BUILD_QT_FRONTEND)
    # Qt Quick usa: Quick (QML engine), Network (HTTP), Gui (gráficos)
    find_package(Qt5 COMPONENTS Quick Network QUIET)

    if(Qt5_FOUND)
        # AUTOMOC: processa Q_OBJECT automaticamente (gera moc_*.cpp)
        # AUTORCC: processa arquivos .qrc automaticamente (embute recursos)
        set(CMAKE_AUTOMOC ON)
        set(CMAKE_AUTORCC ON)

        add_executable(simian_frontend
            qt-frontend/main.cpp
            qt-frontend/dnacontroller.cpp
            qt-frontend/resources.qrc   # Embute o main.qml no executável
        )
        target_link_libraries(simian_frontend
            Qt5::Quick
            Qt5::Network
        )
        message(STATUS "Qt Quick/QML frontend enabled")
    else()
        message(WARNING "Qt5 Quick not found. Install with: sudo apt install qml-module-qtquick2 qml-module-qtquick-controls2 qml-module-qtquick-layouts qtdeclarative5-dev")
    endif()
endif()

# ============================================================================
# Mensagens de ajuda no final do cmake
# ============================================================================
message(STATUS "")
message(STATUS "=== Simian Detector Build Configuration ===")
message(STATUS "  API Server:    Always built (./simian_api)")
message(STATUS "  Tests:         ${BUILD_TESTS} (./simian_tests)")
message(STATUS "  Qt Frontend:   ${BUILD_QT_FRONTEND} (./simian_frontend)")
message(STATUS "")
message(STATUS "Build commands:")
message(STATUS "  mkdir build && cd build")
message(STATUS "  cmake .. -DBUILD_TESTS=ON -DBUILD_QT_FRONTEND=ON")
message(STATUS "  make -j$(nproc)")
message(STATUS "============================================")
