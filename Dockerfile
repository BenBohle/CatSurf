FROM debian:bookworm-slim AS builder

WORKDIR /app

RUN apt-get update && apt-get install -y --no-install-recommends \
    make \
    g++ \
    && rm -rf /var/lib/apt/lists/*

COPY . .
RUN make

FROM debian:bookworm-slim

WORKDIR /app

RUN apt-get update && apt-get install -y --no-install-recommends \
    python3 \
    php-cgi \
    && rm -rf /var/lib/apt/lists/* \
    && mkdir -p /logs

COPY --from=builder /app/webserv /app/webserv
COPY --from=builder /app/config /app/config
COPY --from=builder /app/www /app/www

EXPOSE 8080

CMD ["./webserv", "config/default.conf"]