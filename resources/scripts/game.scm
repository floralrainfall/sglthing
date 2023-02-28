(load "gfx_utils.scm")
(load "game_lib.scm")

(define box-model (quick-load-model "test/box.obj"))
(define box-transform (make-transform))
(define box-texture (quick-load-texture "unuse/tile_t0.png"))
(set! (transform-sx box-transform) 64.0)
(set! (transform-sy box-transform) 1.0)
(set! (transform-sz box-transform) 64.0)
(set! (transform-rz box-transform) 1.0)
(set! (transform-rw box-transform) 0.0)
(update-transform box-transform)

(define box2-transform (make-transform))
(set! (transform-py box2-transform) 3.0)
(set! (transform-sx box2-transform) 5.0)
(set! (transform-sy box2-transform) 5.0)
(set! (transform-sz box2-transform) 5.0)
(set! (transform-rz box2-transform) 1.0)
(set! (transform-rw box2-transform) 0.0)
(update-transform box2-transform)

(define (script-frame world camera) (gfxutils-frame world camera) (gamelib-frame world camera)
    (gamelib-debug-3d-controller camera))

(define (script-frame-render world)
    (gl-bind-texture 0 box-texture)
    (world-draw-object world normal-shader box-model box-transform)
    (world-draw-object world normal-shader box-model box2-transform))

(define (script-frame-ui world)
    (let ((ui-data (world-get-ui world))) (ui-draw-text ui-data 0 0 "Hullo World")))