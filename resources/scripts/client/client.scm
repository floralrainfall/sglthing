(define (client-init network) ())
(define (client-tick network client) 
    (net-send-event network client "Hello World" 0))