---@meta sglthing
-- keep updated with l_functions and script system
-- definition file for lua

world = {}

function world.time(game) return 0.0 end
function world.delta(game) return 0.0 end

ui = {}

function ui.text(ui_ref, x, y, d, text) end 
function ui.image(ui_ref, x, y, w, h, d, texture) end
---@return lightuserdata
function ui.font(ui_ref) end
function ui.font(ui_ref, font) end

asset = {}

function asset.texture(path) return 0 end
---@return lightuserdata
function asset.model(path) end
---@return lightuserdata
function asset.font(path, chara_x, chara_y, chara_w, chara_h) end
function asset.shader(path, type) return 0 end
function asset.program(...) return 0 end

matrix = {}

---@return lightuserdata
function matrix.new() end
function matrix.identity(matrix) end 
function matrix.transform(transform) end 
function matrix.rotate(rotate, w) end 
function matrix.scale(scale) end

gl = {}

gl.fragment_shader = 35632
gl.vertex_shader = 35633

function gl.render_model(game, model, matrix, shader) end
---@return table
function gl.viewport(game) end
function gl.viewport(game, resolution) end