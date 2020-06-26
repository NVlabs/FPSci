class Destination:
    def __init__(self, t, pos):
        self.time = t
        self.pos = pos

class WaypointWriter:
    def __init__(self, filename, targetName = "target", size=[1.0, 1.0]):
        self.file = open(filename, 'w')
        self.targetName = targetName
        self.targetSize = size
        self.file.write('{{\n\tid = "{0}";\n\tdestSpace = "world";\n\tvisualSize = [{1},{2}];\n\tdestinations = (\n'.format(targetName, size[0], size[1]))


    def writeDestination(self, dest):
        self.file.write('\t\t{{\n\t\t\tt = {0};\n\t\t\txyz=Vector3({1},{2},{3});\n\t\t}},\n'.format(dest.time, dest.pos[0], dest.pos[1], dest.pos[2]))

    def writeDestinations(self, dests):
        for dest in dests:
            self.writeDestination(dest)

    def close(self):
        self.file.write("\t);\n}")
        self.file.close()