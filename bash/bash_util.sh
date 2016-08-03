#!/bin/env bash

APP="½£ºÀµÄ³ÌÐò"
USER="½£ºÀ"
MOBILE="jianhao:18905818204"

function sendmail
{
}

function sendww
{
}

function sendsms
{
}

function infomsg
{
	echo "[INFO] [`date +%H:%M:%S`] $1"
}

function warnmsg
{
	echo "[WARN] [`date +%H:%M:%S`] $1"
	sendmail "$1"
	#sendww "$1"
}

function errmsg
{
	echo "[ERROR] [`date +%H:%M:%S`] $1" >&2
	sendmail "$1"
	#sendww "$1"
	sendsms "$1"
}

function rotatelog
{
	local prefix=$1
	local compress=$2
	local dayofweek=$(date -d -1day +%w)

	mv $prefix{,.$dayofweek}

	if [[ $compress -eq 1 ]]; then
		gzip $prefix.$dayofweek
	fi
}

function redirectlog
{
	local log=$1
	exec 1>$log 2>&1
}

# Parameters:
#   $1 command
#   $2 comment for log
#   $3 sleep seconds after each failure
#   $4 retry count
function saferun
{
	local cmd=$1
	local comment=$2
	local slptime=$3
	local retry=$4

	local stat=1

	infomsg "Command: $cmd"
	local i=0
	while [[ 1 ]]; do
		infomsg "$comment"

		$cmd
		stat=$?
		if [[ $stat == 0 ]]; then
			break
		else
			let slptime*=2
			let i++
			if [[ $i -ge $retry ]]; then
				break
			fi
			sleep $slptime
		fi
	done

	issuccess "$stat" "$comment retry $i times"
}

# check the last command if successed
# should be called just after the command you want to be checked
function issuccess
{
	local stat=$1
	local comment=$2
	if [[ $stat != 0 ]]; then
		errmsg "$comment FAILED"
		exit $stat
	else 
		infomsg "$comment SUCCESS"
	fi
}

function rmtexec
{
	local host=$1
	local cmd=$2
	ssh -n $host "$cmd"
}

function foreach
{
	local cmd=$1
	shift

	local -a children
	local i=0

	for h in $@; do
        #replace all {} to hostname
		${cmd//\{\}/$h} &

		children[$i]=$!
		let i++
	done

	for p in ${children[@]}; do
		wait $p
		issuccess $? ""
	done
}

function mrun
{
	local cmd=$1
	shift

	local -a children
	local i=0

	for h in $@; do
		rmtexec $h "$cmd" &

		children[$i]=$!
		let i++
	done

	for p in ${children[@]}; do
		wait $p
		issuccess $? ""
	done
}

function test
{
    echo 'test sendww'
    sendww 'test by jianhao'
    echo ''

	echo 'test function rmtexec'
	rmtexec 'localhost' 'hostname'
	echo ''

	echo 'test foreach'
	foreach 'echo {}' a b c d e f g
	foreach 'sleep {}' 5 2 1
	echo ''

	echo 'test mrun'
	mrun 'sleep 5' 'localhost' 'localhost'
}

test
