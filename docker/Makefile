up:
	wsl --set-default Ubuntu
	wsl -u root apt update
	wsl -u root apt-get upgrade -y
	wsl -u root apt install -y ntpdate
	wsl -u root ntpdate ntp.nict.jp
	docker compose build --progress=plain
	docker compose up -d

exec:
	docker exec -it mk2 bash

up-simply-exec:
	docker compose build --progress=plain
	docker compose up -d

up-exec:
	make up
	make exec

down:
	docker compose down

prune:
	docker image prune -a

rm:
	docker rm $$(docker ps -q)

b-prune:
	docker builder prune

clear:
	make down
	make prune
	make b-prune