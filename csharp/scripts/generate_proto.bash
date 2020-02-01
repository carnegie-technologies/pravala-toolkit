#!/bin/bash

cd $(dirname "${0}")

OUT="../Protocol/Auto.cs"

rm -f "${OUT}"
ProtoGen csharp -f "${OUT}" --id-scope=branch --namespace-prefix=Pravala.Protocol.Auto \
  ../../proto/{ctrl,error,log}.proto
