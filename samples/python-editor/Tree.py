__author__ = "TheComet"

import ik
import pygame
from Updateable import Updateable
from time import time


class Tree(Updateable):
    def __init__(self, root):
        self.root = root
        self.initial_pose = ik.Pose(root)

        font = pygame.font.SysFont(None, 32)

        tstart = time()
        self.solver = ik.Solver(root)
        self.build_img = font.render(f"build() took {time()-tstart}", True, (255, 255, 255))

        self.root_color     = (255, 100, 255)
        self.bone_color     = (100, 100, 255)
        self.effector_color = (200, 255, 0)

        self.grabbed_effector = None
        self.grabbed_root_bone = None

        def get_effectors(bone):
            for child in bone.children:
                yield from get_effectors(child)
            if bone.effector is not None:
                yield bone.effector
        self.effectors = list(get_effectors(root))

        self.bones_img = font.render(f"bones: {self.root.count}", True, (255, 255, 255))
        self.effectors_img = font.render(f"end effectors: {len(self.effectors)}", True, (255, 255, 255))

    def process_event(self, event):
        if event.type == pygame.MOUSEBUTTONDOWN:
            self.grabbed_effector = self.__find_grabbable_effector(event.pos[0], event.pos[1])
            if self.grabbed_effector is not None:
                self.grabbed_effector.target_position.y = event.pos[0]
                self.grabbed_effector.target_position.z = event.pos[1]
                return True

            if (self.root.position.y - event.pos[0])**2 + (self.root.position.z - event.pos[1])**2 < 300:
                self.grabbed_root_bone = self.root
                self.grabbed_root_bone.position.y = event.pos[0]
                self.grabbed_root_bone.position.z = event.pos[1]
                return True

        if event.type == pygame.MOUSEBUTTONUP:
            self.grabbed_effector = None
            self.grabbed_root_bone = None
        if event.type == pygame.MOUSEMOTION:
            if self.grabbed_effector is not None:
                self.grabbed_effector.target_position.y = event.pos[0]
                self.grabbed_effector.target_position.z = event.pos[1]
            if self.grabbed_root_bone is not None:
                self.grabbed_root_bone.position.y = event.pos[0]
                self.grabbed_root_bone.position.z = event.pos[1]

    def __find_grabbable_effector(self, y, z):
        for e in self.effectors:
            if (e.target_position.y-y)**2 + (e.target_position.z-z)**2 < 300:
                return e

    def update(self, time_step):
        #self.initial_pose.apply(self.root)
        tstart = time()
        self.solver.solve()

    def draw(self, surface):
        self.__draw_tree(surface, self.root, ik.Vec3(), ik.Quat())
        self.__draw_effectors(surface)
        surface.blit(self.build_img, (10, 10))
        surface.blit(self.bones_img, (10, 42))
        surface.blit(self.effectors_img, (10, 74))

    def __draw_tree(self, surface, bone, parent_pos, acc_rot):
        # Root bone color is different because it can't be deleted
        color = self.root_color if bone is self.root else self.bone_color

        pos = parent_pos + bone.position * acc_rot

        # ik lib uses 3D coordinates; we're storing our 2D coordinates in the Y and Z components.
        pygame.draw.circle(surface, color, (int(pos.y), int(pos.z)), 4, 1)

        # draw segment
        if bone.parent is not None:
            pygame.draw.line(surface, color, (int(pos.y), int(pos.z)), (int(parent_pos.y), int(parent_pos.z)), 1)

        for child in bone.children:
            self.__draw_tree(surface, child, pos, acc_rot * bone.rotation)

    def __draw_effectors(self, surface):
        for e in self.effectors:
            c = self.effector_color
            tr = ik.Vec3(0, 0, 1) * e.target_rotation * 20
            pygame.draw.circle(surface, c, (int(e.target_position.y), int(e.target_position.z)), 5, 1)
            pygame.draw.line(surface, c, (int(e.target_position.y), int(e.target_position.z)), (int(e.target_position.y + tr.y), int(e.target_position.z + tr.z)), 1)
