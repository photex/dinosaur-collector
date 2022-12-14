
static struct tm_ui_api* tm_ui_api;
static struct tm_draw2d_api* tm_draw2d_api;
static struct tm_the_truth_assets_api* tm_the_truth_assets_api;
static struct tm_creation_graph_api* tm_creation_graph_api;
static struct tm_ui_renderer_api* tm_ui_renderer_api;
static struct tm_error_api* tm_error_api;
static struct tm_the_truth_api* tm_the_truth_api;
static struct tm_temp_allocator_api* tm_temp_allocator_api;
static struct tm_random_api* tm_random_api;

#include <foundation/allocator.h>
#include <foundation/api_registry.h>
#include <foundation/carray.inl>
#include <foundation/error.h>
#include <foundation/macros.h>
#include <foundation/math.inl>
#include <foundation/random.h>
#include <foundation/rect.inl>
#include <foundation/sort.inl>
#include <foundation/temp_allocator.h>
#include <foundation/the_truth.h>
#include <foundation/the_truth_assets.h>

#include <plugins/creation_graph/creation_graph.h>
#include <plugins/creation_graph/creation_graph_output.inl>
#include <plugins/creation_graph/image_nodes.h>
#include <plugins/renderer/render_backend.h>
#include <plugins/renderer/renderer_api_types.h>
#include <plugins/simulate/simulate_entry.h>
#include <plugins/ui/draw2d.h>
#include <plugins/ui/ui.h>
#include <plugins/ui/ui_custom.h>
#include <plugins/ui/ui_renderer.h>

#include <math.h>
#include <memory.h>
#include <stdio.h>

// Implements a dinosaur collecting game.
//
// Static game data is saved in the arrays [[image_paths]], [[props]], [[dinosaurs]], [[drops]] and
// [[mementos]], where as all dynamic game data is saved in the [[tm_simulate_state_o]].

// Helpers

// Returns a `tm_color_srgb_t` corresponding to the hexadecimal color `c`. I.e. `HEXCOLOR(0xff0000)`
// returns a red color. The alpha of the returned color is always set to 255.
#define HEXCOLOR(c) ((tm_color_srgb_t){ .a = 255, .r = 0xff & (c >> 16), .g = 0xff & (c >> 8), .b = 0xff & (c >> 0) })

// Used to specify a numeric range for randomized values.
struct range_t {
    double min, max;
};

// Images

// Index of all images in the game.
enum IMAGE {
    // Used as placeholder for missing graphics.
    PLACEHOLDER,

    // Background images. The background has multiple layers used to implement "hidden surface
    // removal".
    BACKGROUND_LAYER_0,
    BACKGROUND_LAYER_1,
    BACKGROUND_LAYER_2,
    BACKGROUND_LAYER_3,
    BACKGROUND_LAYER_4,

    // Dinosaur images.
    ANKYLOSAURUS,
    ANKYLOSAURUS_2,
    APATOSAURUS,
    BRACHIOSAURUS,
    BRACHIOSAURUS_2,
    CARNOTAURUS,
    DIMORPHODON,
    PACHYCEPHALOSAURUS,
    PARASAUROLOPHUS,
    PARASAUROLOPHUS_2,
    PLESIOSAURUS,
    PLIOSAURUS,
    PTERANODON,
    SPINOSAURUS,
    STEGOSAURUS,
    STEGOSAURUS_2,
    STEGOSAURUS_3,
    STYGIMOLOCH,
    THERIZINOSAURUS,
    TRICERATOPS,
    TRICERATOPS_2,
    TYRANNOSAURUS,
    UTAHCERATOPS,
    VELOCIRAPTOR,

    // Icons
    ALBUM,
    BACK,
    BONE,
    CLOSE,
    INVENTORY,
    MENU,
    MENU_BACKGROUND,
    SHOP,
    SQUARE,
    LEFT_ARROW,
    RIGHT_ARROW,
    MEMENTOS,

    // Props
    BANANA_BUNCH,
    BERRY_BUNCH,
    DEAD_MOUSE,
    FISH,
    HAM,
    HAUNCH,
    HERB_BUNDLE,
    LEAVES,
    MEAT,
    SQUID,
    STARFISH,
    URCHIN,

    // Mementos
    ORE,
    DIAMOND,
    AGATE,
    BRANCH,
    COCONUT,
    DEAD_BIRD,
    FEATHER,
    FERN,
    LAVENDER,
    PEARL,
    SHELL,

    // Total number of images.
    NUM_IMAGES,
};

// Default image to use when no image has been specified.
#define MISSING_ART "art/icons/missing.creation"

// Specifies project paths for the various images. Used to load the image data.
const char* image_paths[NUM_IMAGES] = {
    [PLACEHOLDER] = MISSING_ART,

    [BACKGROUND_LAYER_0] = "art/backgrounds/background.creation",
    [BACKGROUND_LAYER_1] = "art/backgrounds/layer 1.creation",
    [BACKGROUND_LAYER_2] = "art/backgrounds/layer 2.creation",
    [BACKGROUND_LAYER_3] = "art/backgrounds/layer 3.creation",
    [BACKGROUND_LAYER_4] = "art/backgrounds/layer 4.creation",

    [ANKYLOSAURUS] = "art/dinosaurs/ankylosaurus.creation",
    [ANKYLOSAURUS_2] = "art/dinosaurs/ankylosaurus_2.creation",
    [APATOSAURUS] = "art/dinosaurs/apatosaurus.creation",
    [BRACHIOSAURUS] = "art/dinosaurs/brachiosaurus.creation",
    [BRACHIOSAURUS_2] = "art/dinosaurs/brachiosaurus_2.creation",
    [CARNOTAURUS] = "art/dinosaurs/carnotaurus.creation",
    [DIMORPHODON] = "art/dinosaurs/dimorphodon.creation",
    [PACHYCEPHALOSAURUS] = "art/dinosaurs/pachycephalosaurus.creation",
    [PARASAUROLOPHUS] = "art/dinosaurs/parasaurolophus.creation",
    [PARASAUROLOPHUS_2] = "art/dinosaurs/parasaurolophus_2.creation",
    [PLESIOSAURUS] = "art/dinosaurs/plesiosaurus.creation",
    [PLIOSAURUS] = "art/dinosaurs/pliosaurus.creation",
    [PTERANODON] = "art/dinosaurs/pteranodon.creation",
    [SPINOSAURUS] = "art/dinosaurs/spinosaurus.creation",
    [STEGOSAURUS] = "art/dinosaurs/stegosaurus.creation",
    [STEGOSAURUS_2] = "art/dinosaurs/stegosaurus_2.creation",
    [STEGOSAURUS_3] = "art/dinosaurs/stegosaurus_3.creation",
    [STYGIMOLOCH] = "art/dinosaurs/stygimoloch.creation",
    [THERIZINOSAURUS] = "art/dinosaurs/therizinosaurus.creation",
    [TRICERATOPS] = "art/dinosaurs/triceratops.creation",
    [TRICERATOPS_2] = "art/dinosaurs/triceratops_2.creation",
    [TYRANNOSAURUS] = "art/dinosaurs/tyrannosaurus.creation",
    [UTAHCERATOPS] = "art/dinosaurs/utahceratops.creation",
    [VELOCIRAPTOR] = "art/dinosaurs/velociraptor.creation",

    [ALBUM] = "art/icons/album.creation",
    [BACK] = "art/icons/back.creation",
    [BONE] = "art/icons/bone.creation",
    [CLOSE] = "art/icons/close.creation",
    [INVENTORY] = "art/icons/inventory.creation",
    [MENU] = "art/icons/menu.creation",
    [MENU_BACKGROUND] = "art/icons/menu_background.creation",
    [SHOP] = "art/icons/shop.creation",
    [SQUARE] = "art/icons/square.creation",
    [LEFT_ARROW] = "art/icons/left_arrow.creation",
    [RIGHT_ARROW] = "art/icons/right_arrow.creation",
    [MEMENTOS] = "art/icons/mementos.creation",

    [BANANA_BUNCH] = "art/props/banana_bunch.creation",
    [BERRY_BUNCH] = "art/props/berry_bunch.creation",
    [DEAD_MOUSE] = "art/props/dead_mouse.creation",
    [FISH] = "art/props/fish.creation",
    [HAM] = "art/props/ham.creation",
    [HAUNCH] = "art/props/haunch.creation",
    [HERB_BUNDLE] = "art/props/herb_bundle.creation",
    [LEAVES] = "art/props/leaves.creation",
    [MEAT] = "art/props/meat.creation",
    [SQUID] = "art/props/squid.creation",
    [STARFISH] = "art/props/starfish.creation",
    [URCHIN] = "art/props/urchin.creation",

    [ORE] = "art/mementos/ore.creation",
    [DIAMOND] = "art/mementos/diamond.creation",
    [AGATE] = "art/mementos/agate.creation",
    [BRANCH] = "art/mementos/branch.creation",
    [COCONUT] = "art/mementos/coconut.creation",
    [DEAD_BIRD] = "art/mementos/dead_bird.creation",
    [FEATHER] = "art/mementos/feather.creation",
    [FERN] = "art/mementos/fern.creation",
    [LAVENDER] = "art/mementos/lavender.creation",
    [PEARL] = "art/mementos/pearl.creation",
    [SHELL] = "art/mementos/shell.creation",
};

// Props
//
// Props are food you can buy and place in the level to attract dinosaurs. The dinosaurs will
// consume the props and leave a gift.

// Type classification for Props. (Currently, this isn't used for anything.)
enum PROP_TYPE {
    PROP_TYPE__VEG,
    PROP_TYPE__MEAT,
    PROP_TYPE__FISH,
};

// Properties for Props.
struct prop_t {
    // Name of the prop.
    const char* name;

    // Image index for the prop.
    enum IMAGE image;

    // Distance from the bottom of the prop graphics to the bottom of the image box as a fraction of
    // the graphics height (0--1).
    //
    // The prop images are always square and 256 x 256 pixel, with the graphics centered in the
    // square. When we place a prop, we want to align the bottom of the actual graphics of the prop
    // with the cursor position, so we need to offset it by this margin.
    double margin;

    // Scale at which the prop will be drawn. Props can be drawn bigger or smaller in the world.
    double scale;

    // Type of the prop.
    enum PROP_TYPE type;

    // Price to buy the prop in the shop.
    uint32_t price;
};

// List of all the Props in the game.
//
// This list is generated from https://docs.google.com/spreadsheets/d/11sT_7U7IMrL_BpgIoLGul436z4L0lZe-oSCdbEn09DU/edit?pli=1#gid=0
struct prop_t props[] = {
    { .name = "Leaves", .image = LEAVES, .type = PROP_TYPE__VEG, .price = 5, .margin = 0.17, .scale = 0.9 },
    { .name = "Meat", .image = MEAT, .type = PROP_TYPE__MEAT, .price = 5, .margin = 0.2, .scale = 1 },
    { .name = "Fish", .image = FISH, .type = PROP_TYPE__FISH, .price = 5, .margin = 0.35, .scale = 0.7 },
    { .name = "Herb Bundle", .image = HERB_BUNDLE, .type = PROP_TYPE__VEG, .price = 10, .margin = 0.2, .scale = 0.7 },
    { .name = "Banana Bunch", .image = BANANA_BUNCH, .type = PROP_TYPE__VEG, .price = 20, .margin = 0.2, .scale = 0.8 },
    { .name = "Berry Bunch", .image = BERRY_BUNCH, .type = PROP_TYPE__VEG, .price = 30, .margin = 0.2, .scale = 0.6 },
    { .name = "Ham", .image = HAM, .type = PROP_TYPE__MEAT, .price = 10, .margin = 0.2, .scale = 0.9 },
    { .name = "Haunch", .image = HAUNCH, .type = PROP_TYPE__MEAT, .price = 20, .margin = 0.2, .scale = 1.1 },
    { .name = "Dead Mouse", .image = DEAD_MOUSE, .type = PROP_TYPE__MEAT, .price = 30, .margin = 0.2, .scale = 0.7 },
    { .name = "Squid", .image = SQUID, .type = PROP_TYPE__FISH, .price = 10, .margin = 0.2, .scale = 1 },
    { .name = "Urchin", .image = URCHIN, .type = PROP_TYPE__FISH, .price = 20, .margin = 0.2, .scale = 0.7 },
    { .name = "Starfish ", .image = STARFISH, .type = PROP_TYPE__FISH, .price = 30, .margin = 0.2, .scale = 0.7 },
};

// Total number of Props in the game.
#define NUM_PROPS (TM_ARRAY_COUNT(props))

// Dinosaurs

// Type classification for Dinosaurs. This is used to control where dinosaurs can spawn,
// [[DINO_TYPE__ICTYOSAUR]] can only spawn in water and no other dinosaurs can spawn in water.
enum DINO_TYPE {
    DINO_TYPE__HERBIVORE,
    DINO_TYPE__CARNIVORE,
    DINO_TYPE__PTEROSAUR,
    DINO_TYPE__ICTYOSAUR,
};

// Properties for Dinosaurs.
struct dinosaur_t {
    // Name of the dinosaur.
    const char* name;

    // Image for the dinosaur.
    enum IMAGE image;

    // Type of the dinosaur.
    enum DINO_TYPE type;

    // Average minutes before this dinosaur spawns if the right prop is placed.
    double minutes_to_spawn;

    // Images of props that attract this dinosaur. Currently, we only support 1 attracting prop.
    enum IMAGE attracted_by[1];

    // Margin and scale for drawing the dinosaur.
    double margin, scale;
};

// All the Dinosaurs in the game.
//
// Generated from: https://docs.google.com/spreadsheets/d/11sT_7U7IMrL_BpgIoLGul436z4L0lZe-oSCdbEn09DU/edit?pli=1#gid=4726286
struct dinosaur_t dinosaurs[] = {
    { .name = "Ankylosaurus", .image = ANKYLOSAURUS, .type = DINO_TYPE__HERBIVORE, .minutes_to_spawn = 1, .attracted_by = { LEAVES }, .margin = 0.3, .scale = 0.9 },
    { .name = "Ankylosuarus 2", .image = ANKYLOSAURUS_2, .type = DINO_TYPE__HERBIVORE, .minutes_to_spawn = 3, .attracted_by = { HERB_BUNDLE }, .margin = 0.4, .scale = 0.9 },
    { .name = "Apatosaurus", .image = APATOSAURUS, .type = DINO_TYPE__HERBIVORE, .minutes_to_spawn = 5, .attracted_by = { LEAVES }, .margin = 0.03, .scale = 1.2 },
    { .name = "Brachiosaurus", .image = BRACHIOSAURUS, .type = DINO_TYPE__HERBIVORE, .minutes_to_spawn = 10, .attracted_by = { BERRY_BUNCH }, .margin = 0.02, .scale = 1.7 },
    { .name = "Brachiosaurus 2", .image = BRACHIOSAURUS_2, .type = DINO_TYPE__HERBIVORE, .minutes_to_spawn = 30, .attracted_by = { BANANA_BUNCH }, .margin = 0.02, .scale = 1.7 },
    { .name = "Carnotaurus", .image = CARNOTAURUS, .type = DINO_TYPE__CARNIVORE, .minutes_to_spawn = 30, .attracted_by = { HAUNCH }, .margin = 0.13, .scale = 1.2 },
    { .name = "Dimorphodon", .image = DIMORPHODON, .type = DINO_TYPE__PTEROSAUR, .minutes_to_spawn = 10, .attracted_by = { MEAT }, .margin = 0.3, .scale = 0.7 },
    { .name = "Pachycephalosaurus", .image = PACHYCEPHALOSAURUS, .type = DINO_TYPE__HERBIVORE, .minutes_to_spawn = 10, .attracted_by = { BERRY_BUNCH }, .margin = 0.13, .scale = 0.7 },
    { .name = "Parsaurolophus", .image = PARASAUROLOPHUS, .type = DINO_TYPE__HERBIVORE, .minutes_to_spawn = 12, .attracted_by = { BANANA_BUNCH }, .margin = 0.22, .scale = 1 },
    { .name = "Parsaurolophus 2", .image = PARASAUROLOPHUS_2, .type = DINO_TYPE__HERBIVORE, .minutes_to_spawn = 50, .attracted_by = { LEAVES }, .margin = 0.22, .scale = 1 },
    { .name = "Plesiosaurus", .image = PLESIOSAURUS, .type = DINO_TYPE__ICTYOSAUR, .minutes_to_spawn = 45, .attracted_by = { FISH }, .margin = 0.4, .scale = 1 },
    { .name = "Pliosaurus", .image = PLIOSAURUS, .type = DINO_TYPE__ICTYOSAUR, .minutes_to_spawn = 25, .attracted_by = { SQUID }, .margin = 0.3, .scale = 0.7 },
    { .name = "Pteranodon", .image = PTERANODON, .type = DINO_TYPE__PTEROSAUR, .minutes_to_spawn = 20, .attracted_by = { DEAD_MOUSE }, .margin = 0.25, .scale = 0.7 },
    { .name = "Spinosaurus", .image = SPINOSAURUS, .type = DINO_TYPE__CARNIVORE, .minutes_to_spawn = 30, .attracted_by = { HAM }, .margin = 0.25, .scale = 1.5 },
    { .name = "Stegosaurus", .image = STEGOSAURUS, .type = DINO_TYPE__HERBIVORE, .minutes_to_spawn = 5, .attracted_by = { LEAVES }, .margin = 0.3, .scale = 1 },
    { .name = "Stegosaurus 2", .image = STEGOSAURUS_2, .type = DINO_TYPE__HERBIVORE, .minutes_to_spawn = 100, .attracted_by = { HERB_BUNDLE }, .margin = 0.3, .scale = 1 },
    { .name = "Stegosaurus 3", .image = STEGOSAURUS_3, .type = DINO_TYPE__HERBIVORE, .minutes_to_spawn = 200, .attracted_by = { BERRY_BUNCH }, .margin = 0.25, .scale = 0.8 },
    { .name = "Stygimoloch", .image = STYGIMOLOCH, .type = DINO_TYPE__HERBIVORE, .minutes_to_spawn = 120, .attracted_by = { BANANA_BUNCH }, .margin = 0.05, .scale = 0.8 },
    { .name = "Therizinosaurus", .image = THERIZINOSAURUS, .type = DINO_TYPE__CARNIVORE, .minutes_to_spawn = 45, .attracted_by = { URCHIN }, .margin = 0.25, .scale = 1 },
    { .name = "Triceratops", .image = TRICERATOPS, .type = DINO_TYPE__HERBIVORE, .minutes_to_spawn = 10, .attracted_by = { LEAVES }, .margin = 0.25, .scale = 1.2 },
    { .name = "Triceratops 2", .image = TRICERATOPS_2, .type = DINO_TYPE__HERBIVORE, .minutes_to_spawn = 120, .attracted_by = { BERRY_BUNCH }, .margin = 0.25, .scale = 1.2 },
    { .name = "Tyrannosaurus", .image = TYRANNOSAURUS, .type = DINO_TYPE__CARNIVORE, .minutes_to_spawn = 10, .attracted_by = { HAUNCH }, .margin = 0.2, .scale = 1.5 },
    { .name = "Utahceratops", .image = UTAHCERATOPS, .type = DINO_TYPE__HERBIVORE, .minutes_to_spawn = 60, .attracted_by = { BERRY_BUNCH }, .margin = 0.3, .scale = 1.2 },
    { .name = "Velociraptor", .image = VELOCIRAPTOR, .type = DINO_TYPE__CARNIVORE, .minutes_to_spawn = 20, .attracted_by = { HAUNCH }, .margin = 0.1, .scale = 1 },
};

// Total number of Dinosaurs in the game.
#define NUM_DINOSAURS (TM_ARRAY_COUNT(dinosaurs))

// Drops
//
// A Drop is a rule specifying that a certain item is dropped by a certain dinosaur. The dropped
// item can be either a Prop or a Memento.

// Properties for drops.
struct drop_t {
    // Image of the dinosaur that this drop rule concerns.
    enum IMAGE dinosaur_image;

    // Image of the dropped item. We find the actual dropped item by comparing this image with
    // the images in the [[props]] and [[mementos]] lists.
    enum IMAGE drop_image;

    // Quantity of item that will be dropped.
    struct range_t quantity;

    // Probability that item will be dropped.
    double probability;
};

// Rules for drops.
//
// When a dinosaur leaves the game we check each rule. If the dinosaur matches the `dino` field
// we will award the specified (randomized) quantity of drop items with the specified probability.
// (The actual drop number is rounded to the nearest integer.)
//
// Generated from: https://docs.google.com/spreadsheets/d/11sT_7U7IMrL_BpgIoLGul436z4L0lZe-oSCdbEn09DU/edit?pli=1#gid=1632155867
struct drop_t drops[] = {
    { .dinosaur_image = ANKYLOSAURUS, .drop_image = ORE, .quantity = { 1, 1 }, .probability = 1 },
    { .dinosaur_image = ANKYLOSAURUS_2, .drop_image = DIAMOND, .quantity = { 1, 1 }, .probability = 1 },
    { .dinosaur_image = APATOSAURUS, .drop_image = ORE, .quantity = { 1, 1 }, .probability = 1 },
    { .dinosaur_image = BRACHIOSAURUS, .drop_image = BRANCH, .quantity = { 1, 1 }, .probability = 1 },
    { .dinosaur_image = BRACHIOSAURUS_2, .drop_image = COCONUT, .quantity = { 1, 1 }, .probability = 1 },
    { .dinosaur_image = CARNOTAURUS, .drop_image = DEAD_MOUSE, .quantity = { 1, 1 }, .probability = 1 },
    { .dinosaur_image = DIMORPHODON, .drop_image = FEATHER, .quantity = { 1, 1 }, .probability = 1 },
    { .dinosaur_image = PACHYCEPHALOSAURUS, .drop_image = FERN, .quantity = { 1, 1 }, .probability = 1 },
    { .dinosaur_image = PARASAUROLOPHUS, .drop_image = HERB_BUNDLE, .quantity = { 1, 1 }, .probability = 1 },
    { .dinosaur_image = PARASAUROLOPHUS_2, .drop_image = LAVENDER, .quantity = { 1, 1 }, .probability = 1 },
    { .dinosaur_image = PLESIOSAURUS, .drop_image = PEARL, .quantity = { 1, 1 }, .probability = 1 },
    { .dinosaur_image = PLIOSAURUS, .drop_image = SHELL, .quantity = { 1, 1 }, .probability = 1 },
    { .dinosaur_image = PTERANODON, .drop_image = BRANCH, .quantity = { 1, 1 }, .probability = 1 },
    { .dinosaur_image = SPINOSAURUS, .drop_image = FERN, .quantity = { 1, 1 }, .probability = 1 },
    { .dinosaur_image = STEGOSAURUS, .drop_image = DEAD_BIRD, .quantity = { 1, 1 }, .probability = 1 },
    { .dinosaur_image = STEGOSAURUS_2, .drop_image = FEATHER, .quantity = { 1, 1 }, .probability = 1 },
    { .dinosaur_image = STEGOSAURUS_3, .drop_image = COCONUT, .quantity = { 1, 1 }, .probability = 1 },
    { .dinosaur_image = STYGIMOLOCH, .drop_image = LAVENDER, .quantity = { 1, 1 }, .probability = 1 },
    { .dinosaur_image = THERIZINOSAURUS, .drop_image = AGATE, .quantity = { 1, 1 }, .probability = 1 },
    { .dinosaur_image = TRICERATOPS, .drop_image = ORE, .quantity = { 1, 1 }, .probability = 1 },
    { .dinosaur_image = TRICERATOPS_2, .drop_image = BRANCH, .quantity = { 1, 1 }, .probability = 1 },
    { .dinosaur_image = TYRANNOSAURUS, .drop_image = BRANCH, .quantity = { 1, 1 }, .probability = 1 },
    { .dinosaur_image = UTAHCERATOPS, .drop_image = DEAD_BIRD, .quantity = { 1, 1 }, .probability = 1 },
    { .dinosaur_image = VELOCIRAPTOR, .drop_image = DIAMOND, .quantity = { 1, 1 }, .probability = 1 },
};

// Mementos
//
// Mementos are items dropped by dinosaurs that can be sold for cash value.

// Properties for Mementos.
struct memento_t {
    // Name of the memento.
    const char* name;

    // Image index for the memento.
    enum IMAGE image;

    // Price to sell the memento in the shop.
    uint32_t sell_value;
};

// All the Mementos in the game.
//
// Generated from: https://docs.google.com/spreadsheets/d/11sT_7U7IMrL_BpgIoLGul436z4L0lZe-oSCdbEn09DU/edit?pli=1#gid=1102118616
struct memento_t mementos[] = {
    { .name = "Ore", .image = ORE, .sell_value = 1 },
    { .name = "Diamond", .image = DIAMOND, .sell_value = 10 },
    { .name = "Agate", .image = AGATE, .sell_value = 5 },
    { .name = "Branch", .image = BRANCH, .sell_value = 2 },
    { .name = "Coconut", .image = COCONUT, .sell_value = 2 },
    { .name = "Dead bird", .image = DEAD_BIRD, .sell_value = 1 },
    { .name = "Feather", .image = FEATHER, .sell_value = 1 },
    { .name = "Fern", .image = FERN, .sell_value = 2 },
    { .name = "Lavender", .image = LAVENDER, .sell_value = 5 },
    { .name = "Pearl", .image = PEARL, .sell_value = 10 },
    { .name = "Shell", .image = SHELL, .sell_value = 5 },
};

// Total number of Mementos in the game.
#define NUM_MEMENTOS TM_ARRAY_COUNT(mementos)

// Rules

// Rules that control the gameplay.
struct rules_t {
    // Multiplier to the game speed. This can be set to a value > 1 to speed up testing.
    struct range_t speed_multiplier;

    // Money player has at the start.
    struct range_t start_money;

    // Minutes for the player to receive a coin.
    struct range_t minutes_to_coin;

    // Time a dinosaur stays around after it has come to eat food.
    struct range_t dinosaur_lifetime_minutes;

    // Time food stays around if no dinosaur comes to eat it.
    struct range_t food_lifetime_minutes;
};

// Current game rules.
//
// Generated from: https://docs.google.com/spreadsheets/d/11sT_7U7IMrL_BpgIoLGul436z4L0lZe-oSCdbEn09DU/edit?pli=1#gid=702050057
struct rules_t rules = {
    .speed_multiplier = { 1, 1 },
    .start_money = { 100, 100 },
    .minutes_to_coin = { 1, 1 },
    .dinosaur_lifetime_minutes = { 1, 10 },
    .food_lifetime_minutes = { 10, 20 },
};

// Runtime state

// Current state of the game.
enum STATE {
    // The main scene view.
    STATE__MAIN,

    // Menu screen.
    STATE__MENU,

    // Inventory screen.
    STATE__INVENTORY,

    // Shop screen.
    STATE__SHOP,

    // Dinosaur album screen.
    STATE__ALBUM,

    // Placing a prop in the scene.
    STATE__PLACING,

    // Being awarded a gift.
    STATE__AWARD,

    // Mementos screen.
    STATE__MEMENTOS,
};

// Data for a prop placed in the scene.
struct scene_prop_t {
    // Prop.
    const struct prop_t* prop;

    // X and Y position of the prop (in relative coordinates, relative to the background image).
    // I.e. `(0,0)` represents the top left corner of the background image and `(1,1)` the bottom
    // right corner.
    float x, y;

    // Time that this prop has left to live until it disappears.
    double lifetime;
};

// Maximum number of props that can be placed in the scene.
enum { MAX_SCENE_PROPS = 32 };

// Data for a dinosaur placed in the scene.
struct scene_dinosaur_t {
    // Dinosaur.
    const struct dinosaur_t* dinosaur;

    // X and Y position of the dinosaur (in relative coordinates, relative to the background image).
    // I.e. `(0,0)` represents the top left corner of the background image and `(1,1)` the bottom
    // right corner.
    float x, y;

    // If true, the graphics of this dinosaur is horizontally flipped.
    bool flipped;

    // Time that this dinosaur has left to live until it disappears.
    double lifetime;
};

// Maximum number of dinosaurs in the scene.
enum { MAX_SCENE_DINOSAURS = 32 };

// A drop that has been awarded to the player.
struct awarded_drop_t {
    // Dinosaur that awarded the drop.
    const struct dinosaur_t* dinosaur;

    // Number of items (identified by image) in the drop.
    uint32_t quantity[NUM_IMAGES];

    // Total number of dropped items.
    uint32_t total_items;
};

// Maximum number of unclaimed awarded drops that a player can have.
enum { MAX_AWARDED_DROPS = 16 };

// We reserve this many bytes for the game state.
//
// !!! NOTE
//     By reserving `> sizeof(tm_simulate_state_o)` bytes and initializing it to zero, we can
//     add new items to the end of the game state while hot-reloading without crashing. The added
//     items will be zero initialized.
enum { RESERVE_STATE_BYTES = 32 * 1024 };

// Game state.
struct tm_simulate_state_o {
    tm_allocator_i* allocator;

    // Money that the player has.
    uint32_t money;

    // Time until next coin is received.
    double next_coin;

    // Loaded image data.
    uint32_t images[NUM_IMAGES];

    // Current game state.
    enum STATE state;

    // Current page when the STATE is a menu screen.
    uint32_t page;

    // Number of items of each prop type that the player has in her inventory.
    uint32_t inventory[NUM_PROPS];

    // Number of items of each memento type that the player has in her inventory.
    uint32_t mementos[NUM_MEMENTOS];

    // Current scroll amount for main screen. On small displays, the main screen scrolls
    // horizontally to fit the full background image.
    float scroll;

    // In [[STATE__PLACING]] -- the index of the prop that is currently being placed.
    uint32_t place_prop;

    // Props currently paced in the scene.
    uint32_t num_scene_props;
    struct scene_prop_t scene_props[MAX_SCENE_PROPS + 1];

    // Dinosaurs currently in the scene.
    uint32_t num_scene_dinosaurs;
    struct scene_dinosaur_t scene_dinosaurs[MAX_SCENE_DINOSAURS];

    // Dinosaurs that the player has seen.
    bool in_album[NUM_DINOSAURS];

    // Drops that the player hasn't claimed yet.
    uint32_t num_awarded_drops;
    struct awarded_drop_t awarded_drops[MAX_AWARDED_DROPS];
};

// Runtime structs

// Represents an item to draw in the scene.
//
// To draw the scene, we generate a number of [[draw_item_t]], sort them by their
// y-coordinates and draw them in that order.
struct draw_item_t {
    // Y-coordinate for sorting.
    float y;

    // Image to draw for the item.
    enum IMAGE image;

    // Rect where image should be drawn.
    tm_rect_t rect;

    // UV rect for image texture. (If zero, the default (0,0,1,1) will be used.)
    tm_rect_t uv_rect;
};

// Code

// Loads the image at the specified `asset_path` and returns an image handle to it. If the image
// fails to load, the image handle `0` is returned. (This handle is used for the placeholder image.)
static uint32_t load_image(tm_simulate_start_args_t* args, const char* asset_path)
{
    if (!asset_path)
        asset_path = MISSING_ART;

    const tm_tt_id_t asset = tm_the_truth_assets_api->asset_from_path(args->tt, args->asset_root, asset_path);
    if (!TM_ASSERT(asset.u64, tm_error_api->def, "Image not found `%s`", asset_path))
        return 0;

    const tm_tt_id_t object = tm_the_truth_api->get_subobject(args->tt, tm_tt_read(args->tt, asset), TM_TT_PROP__ASSET__OBJECT);
    tm_creation_graph_context_t ctx = (tm_creation_graph_context_t){ .rb = args->render_backend, .device_affinity_mask = TM_RENDERER_DEVICE_AFFINITY_MASK_ALL, .tt = args->tt };
    tm_creation_graph_instance_t inst = tm_creation_graph_api->create_instance(args->tt, object, &ctx);
    tm_creation_graph_output_t output = tm_creation_graph_api->output(&inst, TM_CREATION_GRAPH__IMAGE__OUTPUT_NODE_HASH, &ctx, 0);
    const tm_creation_graph_image_data_t* cg_image = (tm_creation_graph_image_data_t*)output.output;
    const uint32_t image = tm_ui_renderer_api->allocate_image_slot(args->ui_renderer);
    tm_ui_renderer_api->set_image(args->ui_renderer, image, cg_image->handle);
    return image;
}

// Returns `true` if the background-realtive coordinates `(x,y)` are "in the lake". Only
// [[DINO_TYPE__ICTYOSAUR]] can spawn in the lake.
static const bool in_lake(float x, float y)
{
    if (x > 0.39f)
        return false;

    if (y < 0.68f) {
        if (x < 0.09f)
            return y > tm_lerp(0.48f, 0.45f, (x - 0.00f) / 0.09f);
        else if (x < 0.13f)
            return y > tm_lerp(0.45f, 0.52f, (x - 0.09f) / 0.04f);
        else if (x < 0.33f)
            return y > tm_lerp(0.52f, 0.55f, (x - 0.13f) / 0.20f);
        else
            return y > tm_lerp(0.55f, 0.68f, (x - 0.33f) / 0.06f);
    } else {
        if (x < 0.22f)
            return y < 0.88f;
        else
            return y < tm_lerp(0.90f, 0.74f, (x - 0.22f) / 0.17f);
    }
}

// Draws scene props in the array `(draw_props, num_props)`.
static void draw_scene_props(tm_rect_t background_r, struct scene_prop_t* draw_props, uint32_t num_props,
    struct draw_item_t** draw_ptr, tm_temp_allocator_i* ta)
{
    struct draw_item_t* draw = *draw_ptr;
    for (struct scene_prop_t* p = draw_props; p < draw_props + num_props; ++p) {
        const struct prop_t* prop = p->prop;

        const float x = background_r.x + background_r.w * p->x;
        const float y = background_r.y + background_r.h * p->y;

        const float unit = background_r.h;
        const float far_size = 0.06f * unit * (float)prop->scale;
        const float close_size = 0.24f * unit * (float)prop->scale;
        const float rel_size = (p->y - 0.35f) / (1.0f - 0.35f);
        const float size = tm_lerp(far_size, close_size, rel_size);

        if (in_lake(p->x, p->y)) {
            const tm_rect_t r = { x - size / 2, y - size + size * (float)prop->margin, size, size / 2 };
            tm_carray_temp_push(draw, ((struct draw_item_t){ .image = prop->image, .y = p->y, .rect = r, .uv_rect = (tm_rect_t){ 0, 0, 1, 0.5f } }), ta);
        } else {
            const tm_rect_t r = { x - size / 2, y - size + size * (float)prop->margin, size, size };
            tm_carray_temp_push(draw, ((struct draw_item_t){ .image = prop->image, .y = p->y, .rect = r }), ta);
        }
    }
    *draw_ptr = draw;
}

// Draws scene dinosaurs in the array `(draw_dinosaurs, num_dinosaurs)`.
static void draw_scene_dinosaurs(tm_rect_t background_r, struct scene_dinosaur_t* draw_dinosaurs, uint32_t num_dinosaurs,
    struct draw_item_t** draw_ptr, tm_temp_allocator_i* ta)
{
    struct draw_item_t* draw = *draw_ptr;
    for (struct scene_dinosaur_t* d = draw_dinosaurs; d < draw_dinosaurs + num_dinosaurs; ++d) {
        const struct dinosaur_t* dinosaur = d->dinosaur;

        const float x = background_r.x + background_r.w * d->x;
        const float y = background_r.y + background_r.h * d->y;

        const float unit = background_r.h;
        const float far_size = 0.12f * unit * (float)dinosaur->scale;
        const float close_size = 0.48f * unit * (float)dinosaur->scale;
        const float rel_size = (d->y - 0.35f) / (1.0f - 0.35f);
        const float size = tm_lerp(far_size, close_size, rel_size);

        if (in_lake(d->x, d->y)) {
            const tm_rect_t r = { x - size / 2, y - size + size * (float)dinosaur->margin, size, size / 2 };
            const tm_rect_t uv = d->flipped ? (tm_rect_t){ 1, 0, -1, 0.5f } : (tm_rect_t){ 0, 0, 1, 0.5f };
            tm_carray_temp_push(draw, ((struct draw_item_t){ .image = dinosaur->image, .y = d->y, .rect = r, .uv_rect = uv }), ta);
        } else {
            const tm_rect_t r = { x - size / 2, y - size + size * (float)dinosaur->margin, size, size };
            const tm_rect_t uv = d->flipped ? (tm_rect_t){ 1, 0, -1, 1 } : (tm_rect_t){ 0, 0, 1, 1 };
            tm_carray_temp_push(draw, ((struct draw_item_t){ .image = dinosaur->image, .y = d->y, .rect = r, .uv_rect = uv }), ta);
        }
    }
    *draw_ptr = draw;
}

// Rolls a random value in the range and returns it.
static double roll(struct range_t r)
{
    const double t = tm_random_to_double(tm_random_api->next());
    return r.min + t * (r.max - r.min);
}

// Implements the game logic.
static void game_logic(tm_simulate_state_o* state, double dt)
{
    // Time doesn't pass in award screen.
    if (state->state == STATE__AWARD)
        dt = 0;

    // Earn money
    if (!state->next_coin)
        state->next_coin = roll(rules.minutes_to_coin) * 60;
    state->next_coin -= dt;
    if (state->next_coin <= 0) {
        state->money++;
        state->next_coin = 0;
    }

    // Food spoils
    for (uint32_t i = 0; i < state->num_scene_props; ++i) {
        struct scene_prop_t* p = state->scene_props + i;
        if (!p->lifetime)
            p->lifetime = roll(rules.food_lifetime_minutes) * 60.0f;
        p->lifetime -= dt;
        if (p->lifetime <= 0)
            state->scene_props[i--] = state->scene_props[--state->num_scene_props];
    }

    // Dinosaurs walk away
    const struct dinosaur_t* dropping_dino = 0;
    for (uint32_t i = 0; i < state->num_scene_dinosaurs; ++i) {
        struct scene_dinosaur_t* d = state->scene_dinosaurs + i;
        if (!d->lifetime)
            d->lifetime = roll(rules.dinosaur_lifetime_minutes) * 60.0f;
        d->lifetime -= dt;
        if (d->lifetime <= 0) {
            dropping_dino = state->scene_dinosaurs[i].dinosaur;
            state->scene_dinosaurs[i--] = state->scene_dinosaurs[--state->num_scene_dinosaurs];
            break;
        }
    }

    // Drops
    if (dropping_dino) {
        struct awarded_drop_t awarded_drop = { .dinosaur = dropping_dino };

        for (struct drop_t* drop = drops; drop != TM_ARRAY_END(drops); ++drop) {
            if (dropping_dino->image != drop->dinosaur_image)
                continue;

            if (roll((struct range_t){ 0, 1 }) > drop->probability)
                continue;

            const uint32_t quantity = (uint32_t)(roll(drop->quantity) + 0.5f);
            awarded_drop.quantity[drop->drop_image] += quantity;
            awarded_drop.total_items += quantity;
        }

        if (awarded_drop.total_items && state->num_awarded_drops < MAX_AWARDED_DROPS)
            state->awarded_drops[state->num_awarded_drops++] = awarded_drop;
    }

    // Food attracts dinosaurs
    if (state->num_scene_dinosaurs < MAX_SCENE_DINOSAURS) {
        for (uint32_t pi = 0; pi < state->num_scene_props; ++pi) {
            struct scene_prop_t* p = state->scene_props + pi;
            enum IMAGE food = p->prop->image;
            const bool food_in_lake = in_lake(p->x, p->y);

            for (struct dinosaur_t* d = dinosaurs; d != dinosaurs + NUM_DINOSAURS; ++d) {
                if (d->attracted_by[0] != food)
                    continue;

                // Only ICTYOSAURS can spawn in the lake. ICTYOSAURS cannot spawn on land.
                const bool is_ictyosaur = d->type == DINO_TYPE__ICTYOSAUR;
                if (food_in_lake != is_ictyosaur)
                    continue;

                const double spawn_chance = dt / 60 / d->minutes_to_spawn;
                const bool spawn = roll((struct range_t){ 0, 1 }) <= spawn_chance;
                if (!spawn)
                    continue;

                const uint32_t dino_i = (uint32_t)(d - dinosaurs);
                state->in_album[dino_i] = true;
                const struct scene_dinosaur_t dino = { .dinosaur = d, .x = p->x, .y = p->y, .flipped = tm_random_to_bool(tm_random_api->next()) };
                state->scene_dinosaurs[state->num_scene_dinosaurs++] = dino;
                state->scene_props[pi--] = state->scene_props[--state->num_scene_props];
                break;
            }
        }
    }
}

// Draws the scene -- the background layers and the placed props.
static void scene(tm_simulate_state_o* state, tm_simulate_frame_args_t* args)
{
    tm_ui_buffers_t uib = tm_ui_api->buffers(args->ui);
    tm_draw2d_style_t style[1] = { 0 };
    tm_ui_api->to_draw_style(args->ui, style, args->uistyle);
    style->include_alpha = true;
    style->color = (tm_color_srgb_t){ 255, 255, 255, 255 };

    const float aspect = 2.0f;
    tm_rect_t background_r = tm_rect_set_w(args->rect, args->rect.h * aspect);
    if (background_r.w < args->rect.w) {
        background_r.x = (args->rect.w - background_r.w) / 2.0f;
        tm_draw2d_api->fill_rect(uib.vbuffer, *uib.ibuffers, style, args->rect);
        style->clip = tm_draw2d_api->add_clip_rect(uib.vbuffer, background_r);
    } else {
        state->scroll = tm_clamp(state->scroll, 0, background_r.w - args->rect.w);
        if (state->scroll < 0)
            state->scroll = 0;
        background_r.x = -state->scroll;
    }

    uint32_t num_scene_props = state->num_scene_props;
    if (state->state == STATE__PLACING) {
        const float scene_rel_mouse_x = (uib.input->mouse_pos.x - background_r.x) / background_r.w;
        const float scene_rel_mouse_y = (uib.input->mouse_pos.y - background_r.y) / background_r.h;

        const bool is_in_scene = tm_is_between(scene_rel_mouse_x, 0, 1) && tm_is_between(scene_rel_mouse_y, 0.35, 1);
        const bool can_place = is_in_scene;

        if (can_place) {
            state->scene_props[num_scene_props] = (struct scene_prop_t){
                .x = scene_rel_mouse_x,
                .y = scene_rel_mouse_y,
                .prop = props + state->place_prop,
            };
            if (uib.input->left_mouse_pressed) {
                ++state->num_scene_props;
                while (state->num_scene_props > MAX_SCENE_PROPS) {
                    memmove(state->scene_props, state->scene_props + 1, MAX_SCENE_PROPS * sizeof(struct scene_prop_t));
                    state->num_scene_props--;
                }
                --state->inventory[state->place_prop];
                if (state->inventory[state->place_prop] == 0)
                    state->state = STATE__MAIN;
            }
            ++num_scene_props;
        }
    }

    // Draw
    {
        TM_INIT_TEMP_ALLOCATOR(ta);

        // Collect draw calls.
        struct draw_item_t* draw = 0;
        tm_carray_temp_push(draw, ((struct draw_item_t){ .image = BACKGROUND_LAYER_0, .y = 0, .rect = background_r }), ta);
        tm_carray_temp_push(draw, ((struct draw_item_t){ .image = BACKGROUND_LAYER_1, .y = 0.45f, .rect = background_r }), ta);
        tm_carray_temp_push(draw, ((struct draw_item_t){ .image = BACKGROUND_LAYER_2, .y = 0.52f, .rect = background_r }), ta);
        tm_carray_temp_push(draw, ((struct draw_item_t){ .image = BACKGROUND_LAYER_3, .y = 0.82f, .rect = background_r }), ta);
        tm_carray_temp_push(draw, ((struct draw_item_t){ .image = BACKGROUND_LAYER_4, .y = 1, .rect = background_r }), ta);
        draw_scene_props(background_r, state->scene_props, num_scene_props, &draw, ta);
        draw_scene_dinosaurs(background_r, state->scene_dinosaurs, state->num_scene_dinosaurs, &draw, ta);

        // Sort them.
        qsort(draw, tm_carray_size(draw), sizeof(*draw), compare_float);

        // Draw them
        for (struct draw_item_t* d = draw; d != tm_carray_end(draw); ++d) {
            if (d->image) {
                const tm_rect_t uv = d->uv_rect.x == 0 && d->uv_rect.y == 0 && d->uv_rect.w == 0 && d->uv_rect.h == 0 ? (tm_rect_t){ 0, 0, 1, 1 } : d->uv_rect;
                tm_draw2d_api->textured_rect(uib.vbuffer, *uib.ibuffers, style, d->rect, state->images[d->image], uv);
            } else {
                tm_draw2d_style_t rstyle = *style;
                rstyle.color = HEXCOLOR(0xffff00);
                tm_draw2d_api->fill_rect(uib.vbuffer, *uib.ibuffers, &rstyle, d->rect);
            }
        }
        TM_SHUTDOWN_TEMP_ALLOCATOR(ta);
    }

    const float rel_mouse_x = tm_clamp((uib.input->mouse_pos.x - args->rect.x) / args->rect.w, 0, 1);
    if (rel_mouse_x < 0.25f) {
        const float edge_proximity = (0.25f - rel_mouse_x) / 0.25f;
        state->scroll -= args->dt * 2000 * edge_proximity;
    } else if (rel_mouse_x > 0.75f) {
        const float edge_proximity = (rel_mouse_x - 0.75f) / 0.25f;
        state->scroll += args->dt * 2000 * edge_proximity;
    }

    // Enable this to print mouse relative coordinates for testing.
    bool show_mouse_coordinates = false;
    if (show_mouse_coordinates) {
        const float scene_rel_mouse_x = (uib.input->mouse_pos.x - background_r.x) / background_r.w;
        const float scene_rel_mouse_y = (uib.input->mouse_pos.y - background_r.y) / background_r.h;
        char coords_str[128];
        sprintf(coords_str, "(%.2f, %.2f)", scene_rel_mouse_x, scene_rel_mouse_y);
        const tm_rect_t coords_r = { uib.input->mouse_pos.x, uib.input->mouse_pos.y, 32, 32 };
        tm_ui_api->text(args->ui, args->uistyle, &(tm_ui_text_t){ .rect = coords_r, .text = coords_str, .color = &HEXCOLOR(0xff0000) });
    }
}

// Draws the money counter.
static void money(tm_simulate_state_o* state, tm_simulate_frame_args_t* args)
{
    tm_ui_buffers_t uib = tm_ui_api->buffers(args->ui);
    tm_draw2d_style_t style[1] = { 0 };
    tm_ui_api->to_draw_style(args->ui, style, args->uistyle);
    style->color = (tm_color_srgb_t){ .r = 255, .g = 255, .b = 255, .a = 255 };
    style->include_alpha = true;
    const float unit = tm_min(args->rect.w, args->rect.h);

    const float icon_size = 0.08f * unit;
    const float font_scale = icon_size / 18.0f;

    const tm_rect_t inset_r = tm_rect_inset(args->rect, 5, 5);
    const tm_rect_t money_symbol_r = tm_rect_split_bottom(tm_rect_split_left(inset_r, icon_size, 0, 0), icon_size, 0, 1);
    const tm_rect_t money_amount_r = { .x = tm_rect_right(money_symbol_r) + 10, .y = money_symbol_r.y - 2, .w = args->rect.w, .h = icon_size };
    tm_ui_style_t uistyle[1] = { *args->uistyle };
    uistyle->font_scale = font_scale;
    char money_str[64];
    sprintf(money_str, "%d", state->money);
    const tm_rect_t metrics_r = tm_ui_api->text_metrics(uistyle, money_str);
    const tm_rect_t draw_r = { .y = money_symbol_r.y, .w = money_amount_r.x + metrics_r.w, .h = args->rect.h - money_symbol_r.y };
    const tm_rect_t background_r = tm_rect_inset(draw_r, -5, -5);

    tm_draw2d_api->fill_rect(uib.vbuffer, *uib.ibuffers, style, background_r);
    tm_draw2d_api->textured_rect(uib.vbuffer, *uib.ibuffers, style, money_symbol_r, state->images[BONE], (tm_rect_t){ 0, 0, 1, 1 });
    style->color = (tm_color_srgb_t){ .a = 255 };
    tm_ui_api->text(args->ui, uistyle, &(tm_ui_text_t){ .rect = money_amount_r, .text = money_str, .color = &style->color });
}

// Draws a button using the image specified by `image_idx`. Returns `true` if the button was
// clicked.
static bool button(tm_simulate_state_o* state, tm_simulate_frame_args_t* args, tm_rect_t r, const uint32_t image_idx)
{
    const uint64_t id = tm_ui_api->make_id(args->ui);
    tm_ui_buffers_t uib = tm_ui_api->buffers(args->ui);
    tm_draw2d_style_t style[1] = { 0 };
    tm_ui_api->to_draw_style(args->ui, style, args->uistyle);
    style->color = (tm_color_srgb_t){ .r = 255, .g = 255, .b = 255, .a = 255 };
    style->include_alpha = true;

    tm_draw2d_api->textured_rect(uib.vbuffer, *uib.ibuffers, style, r, state->images[image_idx], (tm_rect_t){ 0, 0, 1, 1 });

    if (tm_ui_api->is_hovering(args->ui, r, 0))
        uib.activation->next_hover = id;

    return (uib.activation->hover == id && uib.input->left_mouse_pressed);
}

// Draws a disabled (not clickable) button.
static void disabled_button(tm_simulate_state_o* state, tm_simulate_frame_args_t* args, tm_rect_t r, const uint32_t image_idx)
{
    tm_ui_api->make_id(args->ui);
    tm_ui_buffers_t uib = tm_ui_api->buffers(args->ui);
    tm_draw2d_style_t style[1] = { 0 };
    tm_ui_api->to_draw_style(args->ui, style, args->uistyle);
    style->color = (tm_color_srgb_t){ .r = 255, .g = 255, .b = 255, .a = 64 };
    style->include_alpha = true;

    tm_draw2d_api->textured_rect(uib.vbuffer, *uib.ibuffers, style, r, state->images[image_idx], (tm_rect_t){ 0, 0, 1, 1 });
}

// Returns the name of the gift (Prop or Memento) with the specified image.
static const char* gift_name(enum IMAGE image)
{
    for (struct prop_t* p = props; p != TM_ARRAY_END(props); ++p) {
        if (p->image == image)
            return p->name;
    }

    for (struct memento_t* m = mementos; m != TM_ARRAY_END(mementos); ++m) {
        if (m->image == image)
            return m->name;
    }

    return "Unknown";
}

// Adds the specified gift (Prop or Memento) to the player's inventory.
static void claim_gift(tm_simulate_state_o* state, enum IMAGE image, uint32_t quantity)
{
    for (struct prop_t* p = props; p != TM_ARRAY_END(props); ++p) {
        if (p->image == image)
            state->inventory[p - props] += quantity;
    }

    for (struct memento_t* m = mementos; m != TM_ARRAY_END(mementos); ++m) {
        if (m->image == image)
            state->mementos[m - mementos] += quantity;
    }
}

// Draws the menu screens.
static void menu(tm_simulate_state_o* state, tm_simulate_frame_args_t* args)
{
    tm_ui_buffers_t uib = tm_ui_api->buffers(args->ui);
    tm_draw2d_style_t style[1] = { 0 };
    tm_ui_api->to_draw_style(args->ui, style, args->uistyle);
    style->color = (tm_color_srgb_t){ .r = 255, .g = 255, .b = 255, .a = 255 };
    style->include_alpha = true;
    const float unit = tm_min(args->rect.w, args->rect.h);

    const float icon_size = 0.15f * unit;
    const tm_rect_t inset_r = tm_rect_inset(args->rect, 5, 5);
    const tm_rect_t menu_icon_r = tm_rect_split_top(tm_rect_split_left(inset_r, icon_size, 0, 0), icon_size, 0, 0);

    tm_rect_t rect = args->rect;

    // Click on menu button.
    if (state->state == STATE__MAIN || state->state == STATE__PLACING) {
        if (state->num_awarded_drops)
            state->state = STATE__AWARD;

        if (button(state, args, menu_icon_r, MENU))
            state->state = STATE__MENU;
        return;
    }

    if (state->state != STATE__AWARD && button(state, args, menu_icon_r, CLOSE))
        state->state = STATE__MAIN;

    const tm_rect_t menu_r = tm_rect_center_in(0.8f * unit, 0.8f * unit, args->rect);
    const tm_rect_t close_r = tm_rect_center_in(0.1f * unit, 0.1f * unit, (tm_rect_t){ menu_r.x + 0.02f * unit, menu_r.y + 0.02f * unit });

    tm_draw2d_api->textured_rect(uib.vbuffer, *uib.ibuffers, style, menu_r, state->images[MENU_BACKGROUND], (tm_rect_t){ 0, 0, 1, 1 });

    if (state->state != STATE__AWARD && button(state, args, close_r, state->state == STATE__MENU ? CLOSE : BACK))
        state->state = state->state == STATE__MENU ? STATE__MAIN : STATE__MENU;
    const tm_rect_t menu_inset_r = tm_rect_inset(menu_r, 0.05f * unit, 0.05f * unit);
    rect = menu_inset_r;

    tm_draw2d_style_t highlight = *style;
    highlight.color = HEXCOLOR(0xffff00);

    // Use this to highlight parts of the UI to examine the layout.
    // tm_draw2d_api->fill_rect(uib.vbuffer, *uib.ibuffers, &highlight, price_r);

    // Number of pages in this menu screen.
    uint32_t num_pages = 0;

    if (state->state == STATE__MENU) {
        for (uint32_t idx = 0; idx < 9; ++idx) {
            const uint32_t x = idx % 3;
            const uint32_t y = idx / 3;
            const tm_rect_t row_r = tm_rect_divide_y(rect, 0.04f * unit, 3, y);
            const tm_rect_t icon_r = tm_rect_divide_x(row_r, 0.04f * unit, 3, x);
            switch (idx) {
            case 0:
                if (button(state, args, icon_r, INVENTORY)) {
                    state->state = STATE__INVENTORY;
                    state->page = 0;
                }
                break;
            case 1:
                if (button(state, args, icon_r, SHOP)) {
                    state->state = STATE__SHOP;
                    state->page = 0;
                }
                break;
            case 2:
                if (button(state, args, icon_r, ALBUM)) {
                    state->state = STATE__ALBUM;
                    state->page = 0;
                }
                break;

            case 3:
                if (button(state, args, icon_r, MEMENTOS)) {
                    state->state = STATE__MEMENTOS;
                    state->page = 0;
                }
                break;
            }
        }
    } else if (state->state == STATE__INVENTORY) {
        tm_ui_style_t uistyle[1] = { *args->uistyle };

        uint32_t idx = 0;
        for (uint32_t i = 0;; ++i, ++idx) {
            const uint32_t page = i / 9;
            const uint32_t x = i % 3;
            const uint32_t y = (i % 9) / 3;

            while (idx < NUM_PROPS && state->inventory[idx] == 0)
                ++idx;
            if (idx >= NUM_PROPS)
                break;

            num_pages = page + 1;

            if (state->page != page)
                continue;

            const tm_rect_t row_r = tm_rect_divide_y(rect, 0.01f * unit, 3, y);
            tm_rect_t icon_r = tm_rect_divide_x(row_r, 0.01f * unit, 3, x);

            tm_rect_t inventory_r = tm_rect_split_off_bottom(&icon_r, 0.03f * unit, 0.01f * unit);
            tm_rect_t desc_r = tm_rect_split_off_bottom(&icon_r, 0.03f * unit, 0.01f * unit);
            icon_r = tm_rect_center_in(icon_r.h, icon_r.h, icon_r);
            inventory_r = tm_rect_center_in(icon_r.w, inventory_r.h, inventory_r);

            const tm_color_srgb_t text_color = { .a = 255 };
            uistyle->font_scale = desc_r.h / 18.0f;
            tm_ui_api->text(args->ui, uistyle, &(tm_ui_text_t){ .rect = desc_r, .text = props[idx].name, .color = &text_color, .align = TM_UI_ALIGN_CENTER });

            char inventory_str[32] = { 0 };
            sprintf(inventory_str, "%d", state->inventory[idx]);
            tm_ui_api->text(args->ui, uistyle, &(tm_ui_text_t){ .rect = inventory_r, .text = inventory_str, .color = &text_color, .align = TM_UI_ALIGN_CENTER });

            if (button(state, args, icon_r, props[idx].image)) {
                state->state = STATE__PLACING;
                state->place_prop = idx;
            }
        }
    } else if (state->state == STATE__SHOP) {
        tm_ui_style_t uistyle[1] = { *args->uistyle };

        for (uint32_t idx = 0; idx < NUM_PROPS; ++idx) {
            const uint32_t page = idx / 9;
            const uint32_t x = idx % 3;
            const uint32_t y = (idx % 9) / 3;
            num_pages = page + 1;

            if (state->page != page)
                continue;

            const tm_rect_t row_r = tm_rect_divide_y(rect, 0.01f * unit, 3, y);
            tm_rect_t icon_r = tm_rect_divide_x(row_r, 0.01f * unit, 3, x);

            const bool enabled = state->money >= props[idx].price;

            tm_rect_t price_r = tm_rect_split_off_bottom(&icon_r, 0.03f * unit, 0.01f * unit);
            tm_rect_t desc_r = tm_rect_split_off_bottom(&icon_r, 0.03f * unit, 0.01f * unit);
            icon_r = tm_rect_center_in(icon_r.h, icon_r.h, icon_r);
            price_r = tm_rect_center_in(icon_r.w, price_r.h, price_r);

            const tm_color_srgb_t text_color = { .a = enabled ? 255 : 64 };
            uistyle->font_scale = desc_r.h / 18.0f;
            tm_ui_api->text(args->ui, uistyle, &(tm_ui_text_t){ .rect = desc_r, .text = props[idx].name, .color = &text_color, .align = TM_UI_ALIGN_CENTER });

            const tm_rect_t bone_r = tm_rect_split_off_left(&price_r, price_r.h, 0.01f * unit);
            style->color = (tm_color_srgb_t){ .a = enabled ? 255 : 64, .r = 255, .g = 255, .b = 255 };
            tm_draw2d_api->textured_rect(uib.vbuffer, *uib.ibuffers, style, bone_r, state->images[BONE], (tm_rect_t){ 0, 0, 1, 1 });

            char price_str[32] = { 0 };
            char inventory_str[32] = { 0 };
            sprintf(price_str, "%d", props[idx].price);
            sprintf(inventory_str, "%d", state->inventory[idx]);
            tm_ui_api->text(args->ui, uistyle, &(tm_ui_text_t){ .rect = price_r, .text = price_str, .color = &text_color, .align = TM_UI_ALIGN_LEFT });
            tm_ui_api->text(args->ui, uistyle, &(tm_ui_text_t){ .rect = price_r, .text = inventory_str, .color = &text_color, .align = TM_UI_ALIGN_RIGHT });

            if (!enabled) {
                disabled_button(state, args, icon_r, props[idx].image);
            } else if (button(state, args, icon_r, props[idx].image)) {
                state->money -= props[idx].price;
                state->inventory[idx]++;
            }
        }
    } else if (state->state == STATE__ALBUM) {
        tm_ui_style_t uistyle[1] = { *args->uistyle };

        uint32_t idx = 0;
        for (uint32_t i = 0;; ++i, ++idx) {
            const uint32_t page = i / 9;
            const uint32_t x = i % 3;
            const uint32_t y = (i % 9) / 3;

            while (idx < NUM_DINOSAURS && !state->in_album[idx])
                ++idx;
            if (idx >= NUM_DINOSAURS)
                break;

            num_pages = page + 1;

            if (state->page != page)
                continue;

            const tm_rect_t row_r = tm_rect_divide_y(rect, 0.01f * unit, 3, y);
            tm_rect_t icon_r = tm_rect_divide_x(row_r, 0.01f * unit, 3, x);
            tm_rect_t desc_r = tm_rect_split_off_bottom(&icon_r, 0.03f * unit, 0.01f * unit);
            icon_r = tm_rect_center_in(icon_r.h, icon_r.h, icon_r);

            uistyle->font_scale = desc_r.h / 18.0f;
            const tm_color_srgb_t text_color = { .a = 255 };
            tm_ui_api->text(args->ui, uistyle, &(tm_ui_text_t){ .rect = desc_r, .text = dinosaurs[idx].name, .color = &text_color, .align = TM_UI_ALIGN_CENTER });

            button(state, args, icon_r, dinosaurs[idx].image);
        }
    } else if (state->state == STATE__AWARD) {
        tm_ui_style_t uistyle[1] = { *args->uistyle };

        struct awarded_drop_t* award = state->awarded_drops;

        const tm_color_srgb_t text_color = { .a = 255 };
        tm_rect_t title_r = tm_rect_split_off_top(&rect, 0.03f * unit, 0.01f * unit);
        char gift_text[1024];
        sprintf(gift_text, "%s left you a gift", award->dinosaur->name);
        tm_ui_api->text(args->ui, uistyle, &(tm_ui_text_t){ .rect = title_r, .text = gift_text, .color = &text_color, .align = TM_UI_ALIGN_CENTER });

        uint32_t idx = 0;
        for (uint32_t i = 0;; ++i, ++idx) {
            const uint32_t page = i / 9;
            const uint32_t x = i % 3;
            const uint32_t y = (i % 9) / 3;

            while (idx < NUM_IMAGES && !award->quantity[idx])
                ++idx;
            if (idx >= NUM_IMAGES)
                break;

            num_pages = page + 1;

            if (state->page != page)
                continue;

            const tm_rect_t row_r = tm_rect_divide_y(rect, 0.01f * unit, 3, y);
            tm_rect_t icon_r = tm_rect_divide_x(row_r, 0.01f * unit, 3, x);
            tm_rect_t quantity_r = tm_rect_split_off_bottom(&icon_r, 0.03f * unit, 0.01f * unit);
            tm_rect_t desc_r = tm_rect_split_off_bottom(&icon_r, 0.03f * unit, 0.01f * unit);
            icon_r = tm_rect_center_in(icon_r.h, icon_r.h, icon_r);

            uistyle->font_scale = desc_r.h / 18.0f;

            // Returns the name of the gift (Prop or Memento) with the specified image.
            tm_ui_api->text(args->ui, uistyle, &(tm_ui_text_t){ .rect = desc_r, .text = gift_name(idx), .color = &text_color, .align = TM_UI_ALIGN_CENTER });
            char buffer[32];
            sprintf(buffer, "%d", award->quantity[idx]);
            tm_ui_api->text(args->ui, uistyle, &(tm_ui_text_t){ .rect = quantity_r, .text = buffer, .color = &text_color, .align = TM_UI_ALIGN_CENTER });

            if (button(state, args, icon_r, idx)) {
                const uint32_t q = award->quantity[idx];
                claim_gift(state, idx, q);
                award->quantity[idx] = 0;
                award->total_items -= q;
            }
        }

        if (award->total_items == 0) {
            memmove(state->awarded_drops, state->awarded_drops + 1, sizeof(struct awarded_drop_t) * (MAX_AWARDED_DROPS - 1));
            --state->num_awarded_drops;
        }
        if (state->num_awarded_drops == 0)
            state->state = STATE__MAIN;
    } else if (state->state == STATE__MEMENTOS) {
        tm_ui_style_t uistyle[1] = { *args->uistyle };

        uint32_t idx = 0;
        for (uint32_t i = 0;; ++i, ++idx) {
            const uint32_t page = i / 9;
            const uint32_t x = i % 3;
            const uint32_t y = (i % 9) / 3;

            while (idx < NUM_MEMENTOS && state->mementos[idx] == 0)
                ++idx;
            if (idx >= NUM_MEMENTOS)
                break;

            num_pages = page + 1;

            if (state->page != page)
                continue;

            const tm_rect_t row_r = tm_rect_divide_y(rect, 0.01f * unit, 3, y);
            tm_rect_t icon_r = tm_rect_divide_x(row_r, 0.01f * unit, 3, x);
            tm_rect_t price_r = tm_rect_split_off_bottom(&icon_r, 0.03f * unit, 0.01f * unit);
            tm_rect_t desc_r = tm_rect_split_off_bottom(&icon_r, 0.03f * unit, 0.01f * unit);
            icon_r = tm_rect_center_in(icon_r.h, icon_r.h, icon_r);
            price_r = tm_rect_center_in(icon_r.w, price_r.h, price_r);

            const tm_color_srgb_t text_color = { .a = 255 };
            uistyle->font_scale = desc_r.h / 18.0f;
            tm_ui_api->text(args->ui, uistyle, &(tm_ui_text_t){ .rect = desc_r, .text = mementos[idx].name, .color = &text_color, .align = TM_UI_ALIGN_CENTER });

            const tm_rect_t bone_r = tm_rect_split_off_left(&price_r, price_r.h, 0.01f * unit);
            tm_draw2d_api->textured_rect(uib.vbuffer, *uib.ibuffers, style, bone_r, state->images[BONE], (tm_rect_t){ 0, 0, 1, 1 });

            char price_str[32] = { 0 };
            char inventory_str[32] = { 0 };
            sprintf(price_str, "%d", mementos[idx].sell_value);
            sprintf(inventory_str, "%d", state->mementos[idx]);
            tm_ui_api->text(args->ui, uistyle, &(tm_ui_text_t){ .rect = price_r, .text = price_str, .color = &text_color, .align = TM_UI_ALIGN_LEFT });
            tm_ui_api->text(args->ui, uistyle, &(tm_ui_text_t){ .rect = price_r, .text = inventory_str, .color = &text_color, .align = TM_UI_ALIGN_RIGHT });

            if (button(state, args, icon_r, mementos[idx].image)) {
                --state->mementos[idx];
                state->money += mementos[idx].sell_value;
            }
        }
    }

    // Left and right buttons.
    if (num_pages > 0) {
        state->page = tm_clamp(state->page, 0, num_pages - 1);
        if (state->page > 0) {
            const tm_rect_t left_edge_r = tm_rect_split_left(menu_r, 0.05f * unit, 0, 0);
            const tm_rect_t left_button_r = tm_rect_center_in(unit * 0.15f, unit * 0.15f, left_edge_r);
            if (button(state, args, left_button_r, LEFT_ARROW))
                --state->page;
        }
        if (state->page < num_pages - 1) {
            const tm_rect_t right_edge_r = tm_rect_split_right(menu_r, 0.05f * unit, 0, 1);
            const tm_rect_t right_button_r = tm_rect_center_in(unit * 0.15f, unit * 0.15f, right_edge_r);
            if (button(state, args, right_button_r, RIGHT_ARROW))
                ++state->page;
        }
    }
}

// Implements `tm_simulate_entry_i->start()`.
static tm_simulate_state_o* simulate__start(tm_simulate_start_args_t* args)
{
    TM_STATIC_ASSERT(sizeof(tm_simulate_state_o) < RESERVE_STATE_BYTES);

    tm_simulate_state_o* state = tm_alloc(args->allocator, RESERVE_STATE_BYTES);
    memset(state, 0, RESERVE_STATE_BYTES);
    *state = (tm_simulate_state_o){
        .allocator = args->allocator,
        .money = (uint32_t)roll(rules.start_money),
    };

    for (uint32_t i = 0; i < NUM_IMAGES; ++i)
        state->images[i] = load_image(args, image_paths[i]);

    return state;
}

// Implements `tm_simulate_entry_i->stop()`.
static void simulate__stop(tm_simulate_state_o* state)
{
    tm_allocator_i a = *state->allocator;
    tm_free(&a, state, RESERVE_STATE_BYTES);
}

// Implements `tm_simulate_entry_i->tick()`.
static void simulate__tick(tm_simulate_state_o* state, tm_simulate_frame_args_t* args)
{
    const double speed_multiplier = roll(rules.speed_multiplier);
    game_logic(state, args->dt_unscaled * speed_multiplier);

    scene(state, args);
    money(state, args);
    menu(state, args);
}

// `tm_simulate_entry_i` interface.
static tm_simulate_entry_i simulate_entry_i = {
    .id = TM_STATIC_HASH("tm_dinosaur_simulate", 0xcc5e7b0d0f04fed9ULL),
    .display_name = "Dinosaur Simulate",
    .start = simulate__start,
    .stop = simulate__stop,
    .tick = simulate__tick,
};

// Called to load the plugin.
TM_DLL_EXPORT void tm_load_plugin(struct tm_api_registry_api* reg, bool load)
{
    tm_add_or_remove_implementation(reg, load, TM_SIMULATE_ENTRY_INTERFACE_NAME, &simulate_entry_i);

    tm_ui_api = reg->get(TM_UI_API_NAME);
    tm_draw2d_api = reg->get(TM_DRAW2D_API_NAME);
    tm_the_truth_assets_api = reg->get(TM_THE_TRUTH_ASSETS_API_NAME);
    tm_creation_graph_api = reg->get(TM_CREATION_GRAPH_API_NAME);
    tm_ui_renderer_api = reg->get(TM_UI_RENDERER_API_NAME);
    tm_error_api = reg->get(TM_ERROR_API_NAME);
    tm_the_truth_api = reg->get(TM_THE_TRUTH_API_NAME);
    tm_temp_allocator_api = reg->get(TM_TEMP_ALLOCATOR_API_NAME);
    tm_random_api = reg->get(TM_RANDOM_API_NAME);
}
