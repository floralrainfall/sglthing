; GameLib for sglthing
; This file is Public Domain, but credit would be appreciated!

(define mouse-position '(0.0 0.0))
(define delta-time 0.0)
(define x-axis-input 0.0)
(define y-axis-input 0.0)
(define z-axis-input 0.0)

(define (gamelib-frame world camera)
    (set! mouse-position (input-get-mouse))
    (set! delta-time (world-delta-time world))
    (set! x-axis-input (input-get-axis "x_axis"))
    (set! y-axis-input (input-get-axis "y_axis"))
    (set! z-axis-input (input-get-axis "z_axis")))

(define (gamelib-debug-3d-controller camera)
    (set! (transform-py camera) (+ (transform-py camera) (* y-axis-input (* 10.0 delta-time))))
    (set! (transform-px camera) (+ (transform-px camera) (* x-axis-input (* 10.0 (* (cos (+ (* (transform-ry camera) math-pi-180) math-pi-2))delta-time)))))
    (set! (transform-pz camera) (+ (transform-pz camera) (* x-axis-input (* 10.0 (* (sin (+ (* (transform-ry camera) math-pi-180) math-pi-2)) delta-time)))))
    (set! (transform-px camera) (+ (transform-px camera) (* z-axis-input (* 10.0 (* (cos (* (transform-ry camera) math-pi-180)) delta-time)))))
    (set! (transform-pz camera) (+ (transform-pz camera) (* z-axis-input (* 10.0 (* (sin (* (transform-ry camera) math-pi-180)) delta-time)))))
    (set! (transform-rx camera) (- (transform-rx camera) (* 30.0 (* (cadr mouse-position) delta-time)))) ; pitch
    (set! (transform-ry camera) (+ (transform-ry camera) (* 30.0 (* (car mouse-position) delta-time))))) ; yaw