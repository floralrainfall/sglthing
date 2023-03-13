#!/usr/bin/python3

import requests, time

user_username = "Ryelow"
user_password = "Ryelow"
session_key = ""

def check_key(k):
    url = "https://sgl.endoh.ca/game/auth/check.sgl"
    r = requests.get(url,{"sessionkey":k})
    return (r.content == bytes("1","utf-8"))

def get_username(k):
    url = "https://sgl.endoh.ca/game/auth/key_owner.sgl"
    r = requests.get(url,{"sessionkey":k})
    return str(r.content)    

def get_session(u, p):
    url = "https://sgl.endoh.ca/game/auth/authorize.sgl"
    r = requests.post(url,{"user_username":u,"user_password":p})
    return r.content

def request_game(k):
    u = "https://sgl.endoh.ca/game/launcher/load_game.sgl"
    r = requests.get(u,{"sessionkey":k})
    return int(r.content) # job id

def check_game_status(jobid):
    u = "https://sgl.endoh.ca/game/launcher/game_alive.sgl"
    r = requests.get(u,{"jobid":jobid})
    return bool(r.content)

session_key = get_session(user_username, user_password)
print("our session key " + str(session_key,"utf-8"))

if check_key(session_key):
    username = get_username(session_key)
    print("logged in as %s!" % username)

    job = request_game(session_key)
    while not check_game_status(job):
        time.sleep(1)