FROM ubuntu:latest

RUN apt-get update
RUN apt-get install build-essential -y
RUN apt-get install cmake -y
RUN apt-get install libboost-all-dev -y

WORKDIR /app

COPY . .

RUN cmake . . && cmake --build .

CMD ["./main"]

