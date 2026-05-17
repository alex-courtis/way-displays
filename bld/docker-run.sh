#!/bin/sh

docker run --rm -ti \
	--volume "${PWD}:/way-displays" \
	--workdir="/way-displays" \
	--user "`id -u`:`id -g`" \
	way-displays:latest \
	$@
