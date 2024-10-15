FROM ubuntu:latest

RUN apt-get update
RUN apt-get install build-essential -y
RUN apt-get install cmake -y
RUN apt-get install libboost-all-dev -y
RUN apt-get install postgresql -y
RUN apt-get install libpq-dev -y

WORKDIR /app

COPY . .

RUN cmake . . && cmake --build .

CMD ["./main"]

