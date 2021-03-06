# CmakeLists for directory "applications/meshopt_refinement-app"

# this needs to be set again for each directory
cmake_minimum_required (VERSION 2.8)

# enable compiler output
set (CMAKE_VERBOSE_MAKEFILE ON)

# applications to build
ADD_EXECUTABLE(meshopt_refinement-app meshopt_refinement-app)
TARGET_LINK_LIBRARIES(meshopt_refinement-app feat)

if (FEAT_HAVE_MPI)
  ADD_TEST(sleep1 sleep 2)
  SET_PROPERTY(TEST sleep1 PROPERTY LABELS "mpi,sleep")

  ADD_TEST(meshopt_refinement_app_test_1_mpi_3 ${CMAKE_CTEST_COMMAND}
    --build-and-test "${FEAT_SOURCE_DIR}" "${FEAT_BINARY_DIR}"
    --build-generator ${CMAKE_GENERATOR}
    --build-makeprogram ${CMAKE_MAKE_PROGRAM}
    --build-target meshopt_refinement-app
    --build-nocmake
    --build-noclean
    --test-command ${MPIEXEC} --map-by node ${MPIEXEC_NUMPROC_FLAG} 3 ${MPIEXEC_PREFLAGS} ${FEAT_BINARY_DIR}/applications/meshopt_refinement-app/meshopt_refinement-app --test 1 --mesh-path "${FEAT_SOURCE_DIR}/data/meshes" ${MPIEXEC_POSTFLAGS})
  SET_PROPERTY(TEST meshopt_refinement_app_test_1_mpi_3 PROPERTY LABELS "mpi")
  SET_PROPERTY(TEST meshopt_refinement_app_test_1_mpi_3 PROPERTY FAIL_REGULAR_EXPRESSION "FAILED")
else (FEAT_HAVE_MPI)
  ADD_TEST(meshopt_refinement_app_test_1_serial ${CMAKE_CTEST_COMMAND}
    --build-and-test "${FEAT_SOURCE_DIR}" "${FEAT_BINARY_DIR}"
    --build-generator ${CMAKE_GENERATOR}
    --build-makeprogram ${CMAKE_MAKE_PROGRAM}
    --build-target meshopt_refinement-app
    --build-nocmake
    --build-noclean
    --test-command ${VALGRIND_EXE} ${FEAT_BINARY_DIR}/applications/meshopt_refinement-app/meshopt_refinement-app --test 1 --mesh-path "${FEAT_SOURCE_DIR}/data/meshes" )
  SET_PROPERTY(TEST meshopt_refinement_app_test_1_serial PROPERTY LABELS "serial")
  SET_PROPERTY(TEST meshopt_refinement_app_test_1_serial PROPERTY FAIL_REGULAR_EXPRESSION "FAILED")
endif (FEAT_HAVE_MPI)
