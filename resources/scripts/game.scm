(load "gfx_utils.scm")
(load "game_lib.scm")

(define box-model (quick-load-model "box.obj"))
(define box-transform (make-transform))
(set! (transform-sx box-transform) 5.0)
(set! (transform-sy box-transform) 5.0)
(set! (transform-sz box-transform) 5.0)
(update-transform box-transform)

(define (script-frame world camera) (gfxutils-frame world camera) (gamelib-frame world camera)
    (gamelib-debug-3d-controller camera))

(define (script-frame-render world)
    (world-draw-object world normal-shader box-model box-transform))

(define (script-frame-ui world)
    (let ((ui-data (world-get-ui world))) (ui-draw-text ui-data 0 0 "Hullo World")))