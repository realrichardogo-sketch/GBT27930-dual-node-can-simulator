cmake_minimum_required(VERSION 3.16)

project(gbt27930_sim_lean VERSION 0.1.0 LANGUAGES C)

# Architecture v1.0 (docs/ARCHITECTURE.md): portable C99 everywhere.
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(CTest)

set(GBT_STRICT_FLAGS -std=c99 -pedantic-errors -Wall -Wextra -Werror -Wconversion)

# ---- Protocol core + common types (no HAL, no platform) ----
add_library(gbt_protocol STATIC
    common/gbt_types.c
    protocol/gbt_ring.c
    protocol/gbt_outbox.c
    protocol/gbt_timer.c
    protocol/gbt_id.c
    protocol/gbt_codec.c
    protocol/gbt_bits.c
    protocol/gbt_codec_ext.c
    protocol/gbt_tp.c
    protocol/gbt_safety.c
    protocol/gbt_charger_sm.c
    protocol/gbt_vehicle_sm.c
    protocol/gbt_node.c
)
target_include_directories(gbt_protocol PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_options(gbt_protocol PRIVATE ${GBT_STRICT_FLAGS})

if(BUILD_TESTING)
    # ---- Test support: fake HAL (implements hal/hal.h) and SIL harness ----
    add_library(gbt_fake_hal STATIC tests/support/fake_hal.c)
    target_link_libraries(gbt_fake_hal PUBLIC gbt_protocol)
    target_compile_options(gbt_fake_hal PRIVATE ${GBT_STRICT_FLAGS})

    add_library(gbt_app STATIC app/gbt_integration.c app/sim_plant.c app/battery_model.c)
    target_link_libraries(gbt_app PUBLIC gbt_protocol)
    target_compile_options(gbt_app PRIVATE ${GBT_STRICT_FLAGS})

    add_library(gbt_sil STATIC tests/sil/gbt_sil.c)
    target_link_libraries(gbt_sil PUBLIC gbt_fake_hal gbt_protocol)
    target_compile_options(gbt_sil PRIVATE ${GBT_STRICT_FLAGS})

    function(gbt_add_test name source)
        add_executable(${name} ${source})
        target_link_libraries(${name} PRIVATE ${ARGN})
        target_compile_options(${name} PRIVATE ${GBT_STRICT_FLAGS})
        add_test(NAME ${name} COMMAND ${name})
    endfunction()

    # ---- Linux SocketCAN port (Stage 6B) ----
    add_library(gbt_port_linux STATIC ports/linux/port_linux.c)
    target_link_libraries(gbt_port_linux PUBLIC gbt_protocol)
    target_compile_options(gbt_port_linux PRIVATE ${GBT_STRICT_FLAGS})

    add_library(gbt_linux_node STATIC app/linux_node.c app/gbt_integration.c app/sim_plant.c app/battery_model.c)
    target_link_libraries(gbt_linux_node PUBLIC gbt_port_linux gbt_protocol)
    target_compile_options(gbt_linux_node PRIVATE ${GBT_STRICT_FLAGS})

    foreach(role charger vehicle)
        add_executable(gbt_${role}_node app/linux_${role}_main.c)
        target_link_libraries(gbt_${role}_node PRIVATE gbt_linux_node)
        target_compile_options(gbt_${role}_node PRIVATE ${GBT_STRICT_FLAGS})
    endforeach()

    gbt_add_test(bootstrap_smoke tests/unit/bootstrap_smoke.c)
    gbt_add_test(test_ring       tests/unit/test_ring.c      gbt_protocol)
    gbt_add_test(test_outbox     tests/unit/test_outbox.c    gbt_protocol)
    gbt_add_test(test_timer      tests/unit/test_timer.c     gbt_protocol)
    gbt_add_test(test_id         tests/unit/test_id.c        gbt_protocol)
    gbt_add_test(test_codec      tests/unit/test_codec.c     gbt_protocol)
    gbt_add_test(test_bits       tests/unit/test_bits.c      gbt_protocol)
    gbt_add_test(test_codec_ext  tests/unit/test_codec_ext.c gbt_protocol)
    gbt_add_test(test_tp         tests/unit/test_tp.c        gbt_protocol)
    gbt_add_test(test_hal_fake   tests/unit/test_hal_fake.c  gbt_fake_hal)
    gbt_add_test(test_sil_infra  tests/sil/test_sil_infra.c  gbt_sil)
    gbt_add_test(test_sil_session tests/sil/test_sil_session.c gbt_sil gbt_app gbt_fake_hal)

    gbt_add_test(test_port_linux tests/unit/test_port_linux.c gbt_port_linux)
    # STEP 07-C1.1: shared ISR-safe SPSC RX queue (G-SPSC)
    add_library(gbt_spsc STATIC ports/common/gbt_spsc.c)
    target_include_directories(gbt_spsc PUBLIC ${CMAKE_SOURCE_DIR})
    target_compile_options(gbt_spsc PRIVATE ${GBT_STRICT_FLAGS})
    gbt_add_test(test_spsc        tests/unit/test_spsc.c        gbt_spsc)
    gbt_add_test(test_spsc_signal tests/unit/test_spsc_signal.c gbt_spsc)
    set_tests_properties(test_spsc_signal PROPERTIES TIMEOUT 60)

    # ---- STEP 07-C1.2-B1: G474 FDCAN adapter, host-tested against an FDCAN fake ----
    add_library(gbt_fake_stm32g4 STATIC tests/support/fake_stm32g4/fake_stm32g4.c)
    target_include_directories(gbt_fake_stm32g4 PUBLIC ${CMAKE_SOURCE_DIR}/tests/support/fake_stm32g4)
    target_compile_options(gbt_fake_stm32g4 PRIVATE ${GBT_STRICT_FLAGS})
    add_library(gbt_port_g474_host STATIC ports/stm32/g474/port_g474.c)
    target_link_libraries(gbt_port_g474_host PUBLIC gbt_spsc gbt_fake_stm32g4 gbt_protocol)
    target_compile_options(gbt_port_g474_host PRIVATE ${GBT_STRICT_FLAGS})
    gbt_add_test(test_port_g474 tests/unit/test_port_g474.c gbt_port_g474_host)
    target_sources(test_port_g474 PRIVATE tests/stm32/check_g4_constants.c)
    gbt_add_test(test_port_g474_session tests/sil/test_port_g474_session.c gbt_port_g474_host gbt_app)

    # Same adapter test with AddressSanitizer/UBSan (RX scratch and TX buffer bounds), if available
    include(CheckCSourceCompiles)
    set(CMAKE_REQUIRED_FLAGS "-fsanitize=address,undefined")
    set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=address,undefined")
    check_c_source_compiles("int main(void) { return 0; }" GBT_HAVE_ASAN)
    unset(CMAKE_REQUIRED_FLAGS)
    unset(CMAKE_REQUIRED_LINK_OPTIONS)
    if(GBT_HAVE_ASAN)
        add_executable(test_port_g474_asan tests/unit/test_port_g474.c tests/stm32/check_g4_constants.c
                       ports/stm32/g474/port_g474.c tests/support/fake_stm32g4/fake_stm32g4.c
                       ports/common/gbt_spsc.c)
        target_include_directories(test_port_g474_asan PRIVATE ${CMAKE_SOURCE_DIR}
                                   ${CMAKE_SOURCE_DIR}/tests/support/fake_stm32g4)
        target_link_libraries(test_port_g474_asan PRIVATE gbt_protocol)
        target_compile_options(test_port_g474_asan PRIVATE ${GBT_STRICT_FLAGS}
                               -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
        target_link_options(test_port_g474_asan PRIVATE -fsanitize=address,undefined)
        add_test(NAME test_port_g474_asan COMMAND test_port_g474_asan)
    endif()

    # ---- STEP 07-C2.1: F103 bxCAN adapter, host-tested against a bxCAN fake ----
    add_library(gbt_fake_stm32f1 STATIC tests/support/fake_stm32f1/fake_stm32f1.c)
    target_include_directories(gbt_fake_stm32f1 PUBLIC ${CMAKE_SOURCE_DIR}/tests/support/fake_stm32f1)
    target_compile_options(gbt_fake_stm32f1 PRIVATE ${GBT_STRICT_FLAGS})
    add_library(gbt_port_f103_host STATIC ports/stm32/f103/port_f103.c)
    target_link_libraries(gbt_port_f103_host PUBLIC gbt_spsc gbt_fake_stm32f1 gbt_protocol)
    target_compile_options(gbt_port_f103_host PRIVATE ${GBT_STRICT_FLAGS})
    gbt_add_test(test_port_f103 tests/unit/test_port_f103.c gbt_port_f103_host)
    target_sources(test_port_f103 PRIVATE tests/stm32/check_f1_constants.c)
    gbt_add_test(test_port_f103_session tests/sil/test_port_f103_session.c gbt_port_f103_host gbt_app)
    if(GBT_HAVE_ASAN)
        add_executable(test_port_f103_asan tests/unit/test_port_f103.c tests/stm32/check_f1_constants.c
                       ports/stm32/f103/port_f103.c tests/support/fake_stm32f1/fake_stm32f1.c
                       ports/common/gbt_spsc.c)
        target_include_directories(test_port_f103_asan PRIVATE ${CMAKE_SOURCE_DIR}
                                   ${CMAKE_SOURCE_DIR}/tests/support/fake_stm32f1)
        target_link_libraries(test_port_f103_asan PRIVATE gbt_protocol)
        target_compile_options(test_port_f103_asan PRIVATE ${GBT_STRICT_FLAGS}
                               -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
        target_link_options(test_port_f103_asan PRIVATE -fsanitize=address,undefined)
        add_test(NAME test_port_f103_asan COMMAND test_port_f103_asan)
        add_executable(test_port_f103_session_asan tests/sil/test_port_f103_session.c
                       ports/stm32/f103/port_f103.c tests/support/fake_stm32f1/fake_stm32f1.c
                       ports/common/gbt_spsc.c app/gbt_integration.c app/sim_plant.c app/battery_model.c)
        target_include_directories(test_port_f103_session_asan PRIVATE ${CMAKE_SOURCE_DIR}
                                   ${CMAKE_SOURCE_DIR}/tests/support/fake_stm32f1)
        target_link_libraries(test_port_f103_session_asan PRIVATE gbt_protocol)
        target_compile_options(test_port_f103_session_asan PRIVATE ${GBT_STRICT_FLAGS}
                               -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
        target_link_options(test_port_f103_session_asan PRIVATE -fsanitize=address,undefined)
        add_test(NAME test_port_f103_session_asan COMMAND test_port_f103_session_asan)
    endif()

    # Two-process SocketCAN SIL: exit 77 (interface missing) is reported as NOT RUN, never PASS.
    add_test(NAME linux_sil_vcan
             COMMAND sh ${CMAKE_SOURCE_DIR}/tools/run_linux_sil.sh ${CMAKE_BINARY_DIR}
                     ${CMAKE_BINARY_DIR}/linux_sil vcan0)
    set_tests_properties(linux_sil_vcan PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)

    # ---- Architecture and evidence checks ----
    add_test(NAME architecture_rules
             COMMAND sh ${CMAKE_SOURCE_DIR}/tools/check_architecture.sh ${CMAKE_SOURCE_DIR})
    add_test(NAME protocol_no_writable_statics
             COMMAND sh ${CMAKE_SOURCE_DIR}/tools/check_no_writable_statics.sh
                     ${CMAKE_NM} $<TARGET_OBJECTS:gbt_protocol>
             COMMAND_EXPAND_LISTS)
    add_test(NAME source_manifest
             COMMAND sha256sum -c SOURCES.sha256
             WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/docs/sources)

    find_program(GBT_ARM_GCC arm-none-eabi-gcc)
    if(GBT_ARM_GCC)
        add_test(NAME cross_c99_cortex_m
                 COMMAND sh ${CMAKE_SOURCE_DIR}/tools/check_cross_c99.sh
                         ${GBT_ARM_GCC} ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/cross_c99)
        add_test(NAME cross_c99_spsc
                 COMMAND sh ${CMAKE_SOURCE_DIR}/tools/check_cross_spsc.sh
                         ${GBT_ARM_GCC} ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/cross_spsc)
        # Project1_STEP07 edition: default to the board projects inside this workspace (previously
        # external directories under ~/projects/step06_3_* and ~/projects/step07_stm32_workspace).
        set(GBT_G474_REF "${CMAKE_SOURCE_DIR}/../source/Project1_G474"
            CACHE PATH "G474 CubeMX/Keil project of this workspace (real STM32G4 HAL headers)")
        add_test(NAME cross_c99_g474
                 COMMAND sh ${CMAKE_SOURCE_DIR}/tools/check_cross_g474.sh
                         ${GBT_ARM_GCC} ${CMAKE_SOURCE_DIR} ${GBT_G474_REF} ${CMAKE_BINARY_DIR}/cross_g474)
        set_tests_properties(cross_c99_g474 PROPERTIES SKIP_RETURN_CODE 77)
        set(GBT_F103_REF "${CMAKE_SOURCE_DIR}/../source/Project1_F103B_Receiver_CubeMX"
            CACHE PATH "F103 CubeMX/Keil project of this workspace (real STM32F1 HAL headers)")
        add_test(NAME cross_c99_f103
                 COMMAND sh ${CMAKE_SOURCE_DIR}/tools/check_cross_f103.sh
                         ${GBT_ARM_GCC} ${CMAKE_SOURCE_DIR} ${GBT_F103_REF} ${CMAKE_BINARY_DIR}/cross_f103)
        set_tests_properties(cross_c99_f103 PROPERTIES SKIP_RETURN_CODE 77)
    endif()
endif()
