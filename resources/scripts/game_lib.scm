; GameLib for sglthing
; This file is Public Domain, but credit would be appreciated!

(define mouse-position '(0.0 0.0))
(define delta-time 0.0)
(define x-axis-input 0.0)
(define y-axis-input 0.0)
(define z-axis-input 0.0)

(define dbg-cam-player-pos '(0.0 0.0 0.0))
(define dbg-cam-player-angles '(0.0 0.0))

(define (gamelib-frame world camera)
    (set! mouse-position (input-get-mouse))
    (set! delta-time (world-delta-time world))
    (set! x-axis-input (input-get-axis "x_axis"))
    (set! y-axis-input (input-get-axis "y_axis"))
    (set! z-axis-input (input-get-axis "z_axis")))

(engine-print "GameLib loaded")