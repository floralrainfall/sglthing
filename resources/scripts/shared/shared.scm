
(define (shared-event-0 n c d) (engine-print "Yes sir i received that event"))

(define (server-event-0 n c d) (shared-event-0 n c d))
(define (client-event-0 n c d) (shared-event-0 n c d))