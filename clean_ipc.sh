#!/bin/bash

echo whoami

IPCS_S=`ipcs -s | grep 0x0 | awk '{print $2}'`
IPCS_M=`ipcs -m | grep 'radaw      666' | awk '{print $2}'`
IPCS_Q=`ipcs -q | grep 0x0 | awk '{print $2}'`

echo "IPCS_S:\n$IPCS_S"
echo "IPCS_M:\n$IPCS_M"
echo "IPCS_Q:\n$IPCS_Q"

for id in $IPCS_M; do
  ipcrm -m $id;
done

for id in $IPCS_S; do
  ipcrm -s $id;
done

for id in $IPCS_Q; do
  ipcrm -q $id;
done
