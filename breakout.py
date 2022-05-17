"""
A fun implmenetation of atari's breakout using your face in the webcam as the actual controller
"""
import cv2 as cv
import pygame
from pygame.locals import *
from pygame import Rect, Vector2 as vec
import face_recognition as fr


BALL_SPEED = 8


class Player:
    """ A class representing the player and all of its motions

    Parameters
    ----------
    - pos: the starting position of the player
    """
    def __init__(self, pos):
        self.rect = Rect(pos, (100, 10))

    def update(self, game):
        """ updates the player on every frame before being drawn

        Parameters
        ----------
        - game (Game): an instances of the Game class in order to access the game elements such as the current frame
        """
        frame = cv.rotate(game.frame, cv.ROTATE_90_CLOCKWISE)
        faces = fr.face_locations(frame[:,::-1])
        if faces:
            top, right, bottom, left = faces[0]
            center = (right + left) / 2
            self.rect.update(center - self.rect.width/2, self.rect.top, self.rect.width, self.rect.height)

    def draw(self, screen):
        """ draws the player to the screen

        Parameters
        ----------
        - screen (Surface): the screen to draw on
        """
        pygame.draw.rect(screen, (255, 255, 255), self.rect)


class Ball:
    def __init__(self, pos, r):
        self.r = r
        self.rect = pygame.Rect(pos.elementwise() - r, (r*2, r*2))
        self.vel = vec(0, -BALL_SPEED)

    def update(self, game):
        pass

    def draw(self, screen):
        pygame.draw.circle(screen, (255, 0, 0), self.rect.center, self.r)


class StateBall(Ball):
    def __init__(self, ball):
        super().__init__(vec(ball.rect.center), ball.r)


class PlayBall(StateBall):
    def update(self, game):
        bounding_rect = game.screen.get_bounding_rect()
        next_rect = self.rect.move(self.vel)
        if next_rect.left < bounding_rect.left or next_rect.right > bounding_rect.right:
            self.vel.x *= -1
        if next_rect.top < bounding_rect.top:
            self.vel.y *= -1
        if next_rect.bottom > bounding_rect.bottom:
            game.set_state("standby")
            return

        collide_id = next_rect.collidelist(list(map(lambda x: x[0], game.bricks)))
        if collide_id >= 0:
            collided, _ = game.bricks[collide_id]
            if next_rect.centerx < collided.left or next_rect.centerx > collided.right:
                self.vel.x *= -1
            elif next_rect.centery < collided.top or next_rect.centery > collided.bottom:
                self.vel.y *= -1

            if collide_id == len(game.bricks) - 1:
                dist = self.rect.centerx - collided.centerx
                percent = dist / (collided.centerx - collided.left)
                angle = percent * 75 + 270
                self.vel.from_polar((self.vel.magnitude(), angle))
            game.remove_brick(collide_id)

        self.rect.move_ip(self.vel)


class StandbyBall(StateBall):
    def update(self, game):
        player = game.player
        self.rect.update(player.rect.centerx - self.r, player.rect.top - self.r*2, self.rect.width, self.rect.height)


class Game:
    def __init__(self, width=400, height=300):
        self.width = width
        self.height = height
        self.cap = cv.VideoCapture()
        self.cap.open(0)
        self.frame = None
        pygame.init()
        self.screen = pygame.display.set_mode((width, height))
        self.clock = pygame.time.Clock()
        self.player = Player(vec(self.width//2, self.height - 20))
        self.ball = Ball(vec(self.width//2 - 25, self.height//2), 10.)
        self.bricks = self.generate_bricks()

        self.key_down = {}
        self.state = None
        self.set_state("standby")


    def generate_bricks(self):
        bricks = []
        colors = [(255, 0, 0), (127, 127, 0), (0, 127, 0), (0, 0, 128)]
        for col in range(0, self.width, 50):
            for color, row in zip(colors, range(0, 40, 10)):
                bricks.append((Rect(col, row, 50, 10), color))
        return bricks + [(self.player.rect, (0, 0, 255))]

    def remove_brick(self, brick_id):
        if brick_id < len(self.bricks) - 1:
            del self.bricks[brick_id]
        if len(self.bricks) <= 1:
            self.bricks = self.generate_bricks()
            self.set_state("standby")

    def set_state(self, state):
        if state == "play" and self.state != "play":
            self.ball = PlayBall(self.ball)
            self.state = "play"
        elif state == "standby" and self.state != "standby":
            self.ball = StandbyBall(self.ball)
            self.state = "standby"

    def is_key_down(self, key):
        return self.key_down.get(key, False)

    def handle_events(self):
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                return False
            if event.type == pygame.KEYDOWN:
                self.key_down[pygame.key.name(event.key)] = True
            if event.type == pygame.KEYUP:
                self.key_down[pygame.key.name(event.key)] = False
        return True

    def update(self):
        if self.is_key_down("q"):
            return False
        if self.is_key_down("space"):
            self.set_state("play")
        _, self.frame = self.cap.read()
        self.frame = cv.resize(self.frame, (self.width, self.height))[:,:,::-1]
        self.frame = cv.rotate(self.frame, cv.ROTATE_90_COUNTERCLOCKWISE)
        self.player.update(self)
        self.ball.update(self)
        return True

    def draw(self):
        pygame.surfarray.blit_array(self.screen, self.frame)
        for (brick, color) in self.bricks:
            pygame.draw.rect(self.screen, color, brick)
        self.ball.draw(self.screen)
        return True

    def mainloop(self):
        while self.handle_events() and self.update() and self.draw():
            pygame.display.flip()
            self.clock.tick(30)


def main():
    Game().mainloop()


if __name__ == "__main__":
    main()
