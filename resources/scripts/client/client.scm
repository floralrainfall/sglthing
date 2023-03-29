(define (client-init network) ())
(define (client-tick network client) 
    (net-send-event network client "Yop wahts upp" 0))