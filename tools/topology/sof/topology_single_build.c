----- Budowanie pojedynczej topologii -----
------------------------------------------
*** Build of .conf file ***

cd /home/admin/Dev/fork/sof/tools/build_tools/topology && m4 -DPLATFORM=cml -I /home/admin/Dev/fork/sof/tools/topology/m4 -I /home/admin/Dev/fork/sof/tools/topology/common -I /home/admin/Dev/fork/sof/tools/topology/platform/common -I /home/admin/Dev/fork/sof/tools/topology -I /home/admin/Dev/fork/sof/tools/build_tools/topology /home/admin/Dev/fork/sof/tools/topology/common/abi.m4 /home/admin/Dev/fork/sof/tools/topology/sof-cml-rt5682-max98357a.m4 > sof-cml-rt5682-max98357a.conf

*** Build of .tplg file ***
alsatplg -v 1 -c sof-cml-rt5682-max98357a.conf -o sof-cml-rt5682-max98357a.tplg


---- NOTES / ADDITIONAL INFO ----
--------------------------------

_toplevel_:4734:30 -> This error means sth wrong happens in the input file at line 4734 and column 30
